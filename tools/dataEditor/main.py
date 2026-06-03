import os
import json
import copy
import tkinter as tk
from tkinter import ttk, messagebox, simpledialog

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '..', '..', 'src', 'data'))
I18N_DIR = os.path.join(DATA_DIR, 'i18n')

class ToolTip:
    def __init__(self, widget):
        self.widget = widget
        self.tipwindow = None
        self.text = ""

    def showtip(self, text):
        self.text = text
        if self.tipwindow or not self.text:
            return
        x, y, cx, cy = self.widget.bbox("insert")
        x += self.widget.winfo_rootx() + 30
        y += cy + self.widget.winfo_rooty() + 20
        self.tipwindow = tw = tk.Toplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.wm_geometry(f"+{x}+{y}")
        label = tk.Label(tw, text=self.text, justify=tk.LEFT, background="#ffffe0", foreground="#000000", relief=tk.SOLID, borderwidth=1, font=("tahoma", "8", "normal"))
        label.pack(ipadx=4, ipady=2)

    def hidetip(self):
        if self.tipwindow:
            self.tipwindow.destroy()
            self.tipwindow = None

class AutocompleteEntry(ttk.Combobox):
    def __init__(self, parent, completion_dict, *args, **kwargs):
        super().__init__(parent, *args, **kwargs)
        self.completion_dict = completion_dict
        self.completion_list = sorted(list(self.completion_dict.keys()), key=str.lower)
        self['values'] = self.completion_list
        self.bind('<KeyRelease>', self.handle_keyrelease)
        
    def handle_keyrelease(self, event):
        if event.keysym in ("Up", "Down", "Left", "Right", "Return", "Escape", "Tab"):
            return
        typed = self.get()
        if typed == '':
            self['values'] = self.completion_list
        else:
            self['values'] = [item for item in self.completion_list if typed.lower() in item.lower()]
        self.tk.call('ttk::combobox::Post', self)
        
    def get_actual_value(self):
        val = self.get()
        return self.completion_dict.get(val, val)

class JSONEditorApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.geometry("1200x800")
        
        style = ttk.Style(self)
        if "clam" in style.theme_names():
            style.theme_use("clam")
            
        # Ensure Treeview tag background colors work correctly across different themes
        style.map("Treeview",
                  background=[("selected", "#4a6984")],
                  foreground=[("selected", "white")])
            
        self.current_file = None
        self.current_data = None
        
        self.ui_i18n_data = {}
        self.preview_i18n_data = {}
        self.available_langs = self._get_available_langs()
        
        default_lang = self.available_langs[0] if self.available_langs else ""
        self.current_ui_lang = tk.StringVar(value=default_lang)
        self.current_preview_lang = tk.StringVar(value=default_lang)
        
        self._search_results = []
        self._search_idx = -1
        self._last_search_query = ""
        
        self.tooltip = None
        self.last_hovered_item = None
        self.selected_json_node = None
        
        self._setup_ui()
        self.load_ui_i18n_data()
        self.load_preview_i18n_data()
        self.update_ui_texts()
        self.populate_file_tree()
        
        self.bind("<Control-s>", lambda e: self.save_file())
        self.bind("<Control-r>", lambda e: self.reload_file())

    def _get_available_langs(self):
        langs = set()
        
        if os.path.exists(I18N_DIR): 
            for d in os.listdir(I18N_DIR):
                if os.path.isdir(os.path.join(I18N_DIR, d)):
                    langs.add(d)
                    
        editor_i18n_dir = os.path.join(SCRIPT_DIR, 'i18n')
        if os.path.exists(editor_i18n_dir):
            for f in os.listdir(editor_i18n_dir):
                if f.endswith(".json"):
                    langs.add(f[:-5])
                    
        langs_list = list(langs)
        
        if "zh" in langs_list: langs_list.remove("zh"); langs_list.insert(0, "zh")
        elif "en" in langs_list: langs_list.remove("en"); langs_list.insert(0, "en")
        
        return langs_list

    def _setup_ui(self):
        self.paned = ttk.PanedWindow(self, orient=tk.HORIZONTAL)
        self.paned.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)
        
        self.left_frame = ttk.Frame(self.paned)
        self.paned.add(self.left_frame, weight=1)
        self.lbl_data_files = ttk.Label(self.left_frame, font=("Arial", 11, "bold"))
        self.lbl_data_files.pack(anchor=tk.W, pady=(0, 5))
        tree_scroll = ttk.Scrollbar(self.left_frame)
        tree_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.file_tree = ttk.Treeview(self.left_frame, show="tree", yscrollcommand=tree_scroll.set)
        self.file_tree.pack(fill=tk.BOTH, expand=True)
        tree_scroll.config(command=self.file_tree.yview)
        self.file_tree.bind("<<TreeviewSelect>>", self.on_file_select)

        self.right_frame = ttk.Frame(self.paned)
        self.paned.add(self.right_frame, weight=3)
        
        top_bar = ttk.Frame(self.right_frame)
        top_bar.pack(fill=tk.X, pady=(0, 5))
        self.file_label = ttk.Label(top_bar, font=("Arial", 11, "bold"), foreground="#666")
        self.file_label.pack(side=tk.LEFT)
        self.btn_save = ttk.Button(top_bar, command=self.save_file)
        self.btn_save.pack(side=tk.RIGHT, padx=5)
        self.btn_reload = ttk.Button(top_bar, command=self.reload_file)
        self.btn_reload.pack(side=tk.RIGHT)
        
        if self.available_langs:
            self.preview_lang_combo = ttk.Combobox(top_bar, textvariable=self.current_preview_lang, values=self.available_langs, state="readonly", width=5)
            self.preview_lang_combo.pack(side=tk.RIGHT, padx=(0, 5))
            self.preview_lang_combo.bind("<<ComboboxSelected>>", self.on_preview_lang_change)
            self.lbl_preview = ttk.Label(top_bar)
            self.lbl_preview.pack(side=tk.RIGHT, padx=(5, 2))
            
            self.ui_lang_combo = ttk.Combobox(top_bar, textvariable=self.current_ui_lang, values=self.available_langs, state="readonly", width=5)
            self.ui_lang_combo.pack(side=tk.RIGHT, padx=(0, 10))
            self.ui_lang_combo.bind("<<ComboboxSelected>>", self.on_ui_lang_change)
            self.lbl_ui_lang = ttk.Label(top_bar)
            self.lbl_ui_lang.pack(side=tk.RIGHT, padx=(5, 2))

        search_frame = ttk.Frame(self.right_frame)
        search_frame.pack(fill=tk.X, pady=(0, 5))
        self.lbl_search = ttk.Label(search_frame)
        self.lbl_search.pack(side=tk.LEFT, padx=(0, 5))
        
        self.search_var = tk.StringVar()
        self.search_var.trace_add("write", self.on_search_change)
        self.search_entry = ttk.Entry(search_frame, textvariable=self.search_var)
        self.search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.search_entry.bind("<Return>", lambda e: self.find_next())
        self.search_entry.bind("<Shift-Return>", lambda e: self.find_prev())
        
        self.lbl_search_count = ttk.Label(search_frame, text="0/0", width=8, anchor=tk.CENTER)
        self.lbl_search_count.pack(side=tk.LEFT, padx=5)
        
        self.btn_find_prev = ttk.Button(search_frame, command=self.find_prev, width=6)
        self.btn_find_prev.pack(side=tk.LEFT, padx=2)
        self.btn_find_next = ttk.Button(search_frame, command=self.find_next, width=6)
        self.btn_find_next.pack(side=tk.LEFT, padx=2)

        columns = ("Value", "Type", "RawValue")
        self.json_tree = ttk.Treeview(self.right_frame, columns=columns, show="tree headings")
        self.json_tree.column("#0", width=250, minwidth=150)
        self.json_tree.column("Value", width=400, minwidth=200)
        self.json_tree.column("Type", width=80, stretch=False, anchor=tk.CENTER)
        self.json_tree.column("RawValue", width=0, stretch=False)
        self.json_tree["displaycolumns"] = ("Value", "Type")
        self.json_tree.tag_configure("search_match", background="#ffeb99")
        
        y_scroll = ttk.Scrollbar(self.right_frame, orient=tk.VERTICAL, command=self.json_tree.yview)
        self.json_tree.configure(yscrollcommand=y_scroll.set)
        y_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.json_tree.pack(fill=tk.BOTH, expand=True)
        
        self.json_tree.bind("<ButtonRelease-1>", self.on_json_click)
        self.json_tree.bind("<Double-1>", self.on_json_double_click)
        self.json_tree.bind("<Motion>", self.on_json_hover)
        self.json_tree.bind("<Leave>", self.on_json_leave)
        
        self.tooltip = ToolTip(self.json_tree)
        
        self.context_menu = tk.Menu(self, tearoff=0)
        self.context_menu.add_command(command=lambda: self.add_child("str"))
        self.context_menu.add_command(command=lambda: self.add_child("dict"))
        self.context_menu.add_command(command=lambda: self.add_child("list"))
        self.context_menu.add_separator()
        self.context_menu.add_command(command=self.duplicate_item)
        self.context_menu.add_command(command=self.delete_item)
        self.json_tree.bind("<Button-3>", self.post_context_menu)

    def _load_i18n_dict(self, lang):
        data_dict = {}
        if not lang: return data_dict
        
        editor_i18n_dir = os.path.join(SCRIPT_DIR, 'i18n')
        lang_file = os.path.join(editor_i18n_dir, f"{lang}.json")
        if os.path.exists(lang_file):
            try:
                with open(lang_file, 'r', encoding='utf-8') as file:
                    d = json.load(file)
                    if isinstance(d, dict): data_dict.update(d)
            except Exception:
                pass
                
        game_lang_dir = os.path.join(I18N_DIR, lang)
        if os.path.exists(game_lang_dir):
            for f in os.listdir(game_lang_dir):
                if f.endswith(".json"):
                    try:
                        with open(os.path.join(game_lang_dir, f), 'r', encoding='utf-8') as file:
                            d = json.load(file)
                            if isinstance(d, dict): data_dict.update(d)
                    except Exception:
                        pass
                        
        return data_dict

    def load_ui_i18n_data(self):
        self.ui_i18n_data = self._load_i18n_dict(self.current_ui_lang.get())

    def load_preview_i18n_data(self):
        self.preview_i18n_data = self._load_i18n_dict(self.current_preview_lang.get())

    def get_ui_text(self, key, default_text):
        return self.ui_i18n_data.get(key, default_text)

    def update_ui_texts(self):
        self.title(self.get_ui_text("editor.title", "XMenu Editor"))
        
        self.lbl_data_files.config(text=self.get_ui_text("editor.dataFiles", "Data Files"))
        self.btn_save.config(text=self.get_ui_text("editor.save", "Save File"))
        self.btn_reload.config(text=self.get_ui_text("editor.reload", "Reload"))
        
        if hasattr(self, 'lbl_ui_lang'): 
            self.lbl_ui_lang.config(text=self.get_ui_text("editor.uiLang", "UI:"))
            self.lbl_preview.config(text=self.get_ui_text("editor.previewLang", "Preview:"))
            
        self.lbl_search.config(text=self.get_ui_text("editor.search", "Search:"))
        self.btn_find_prev.config(text=self.get_ui_text("editor.btnPrev", "Prev"))
        self.btn_find_next.config(text=self.get_ui_text("editor.btnNext", "Next"))
        
        if not self.current_file:
            self.file_label.config(text=self.get_ui_text("editor.noFile", "No file selected"))
            
        self.json_tree.heading("#0", text=self.get_ui_text("editor.colKey", "Key / Index"))
        self.json_tree.heading("Value", text=self.get_ui_text("editor.colValue", "Value"))
        self.json_tree.heading("Type", text=self.get_ui_text("editor.colType", "Type"))
        self.json_tree.heading("RawValue", text=self.get_ui_text("editor.colRaw", "Raw Value"))
        
        self.context_menu.entryconfig(0, label=self.get_ui_text("editor.ctxAddStr", "Add Property (String)"))
        self.context_menu.entryconfig(1, label=self.get_ui_text("editor.ctxAddDict", "Add Property (Dict)"))
        self.context_menu.entryconfig(2, label=self.get_ui_text("editor.ctxAddList", "Add Property (List)"))
        self.context_menu.entryconfig(4, label=self.get_ui_text("editor.ctxDuplicate", "Duplicate Item"))
        self.context_menu.entryconfig(5, label=self.get_ui_text("editor.ctxDelete", "Delete"))

    def on_ui_lang_change(self, event=None):
        self.load_ui_i18n_data()
        self.update_ui_texts()

    def on_preview_lang_change(self, event=None):
        self.load_preview_i18n_data()
        if self.current_file:
            expanded_nodes = self._get_expanded_nodes("", [])
            selected_path = self._get_node_path(self.selected_json_node) if self.selected_json_node else None
            self.render_json_tree()
            self._restore_expanded_nodes("", expanded_nodes)
            if selected_path: self._restore_selected_node("", selected_path)
            
            # Reapply search tags
            if self.search_var.get():
                self.on_search_change()

    def populate_file_tree(self):
        self.file_tree.delete(*self.file_tree.get_children())
        if not os.path.exists(DATA_DIR):
            messagebox.showerror("Error", f"{self.get_ui_text('editor.errNoDir', 'Data directory not found:')}\n{DATA_DIR}")
            return
            
        node_dict = {"": ""}
        for root, dirs, files in os.walk(DATA_DIR):
            rel_path = os.path.relpath(root, DATA_DIR)
            parent_node = "" if rel_path == "." else ""
            if rel_path != ".":
                parts = rel_path.split(os.sep)
                for p in parts:
                    node_id = parent_node + "/" + p if parent_node else p
                    if node_id not in node_dict:
                        node_dict[node_id] = self.file_tree.insert(node_dict[parent_node], "end", text=p, open=True)
                    parent_node = node_id
            for f in files:
                if f.endswith(".json"):
                    f_path = os.path.join(root, f)
                    node_id = parent_node + "/" + f if parent_node else f
                    self.file_tree.insert(node_dict[parent_node], "end", text=f, values=(f_path,))

    def on_file_select(self, event):
        selection = self.file_tree.selection()
        if not selection: return
        item = self.file_tree.item(selection[0])
        values = item.get("values")
        if values: self.load_file(values[0])

    def load_file(self, file_path):
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                self.current_data = json.load(f)
            self.current_file = file_path
            self.file_label.config(text=os.path.basename(file_path), foreground="#000")
            self.render_json_tree()
            self.selected_json_node = None
            if self.search_var.get():
                self.on_search_change()
        except Exception as e:
            messagebox.showerror("Error", f"{self.get_ui_text('editor.errLoad', 'Failed to load JSON:')}\n{e}")

    def reload_file(self):
        if self.current_file: self.load_file(self.current_file)

    def save_file(self):
        if not self.current_file or self.current_data is None: return
        try:
            with open(self.current_file, 'w', encoding='utf-8') as f:
                json.dump(self.current_data, f, indent=2, ensure_ascii=False)
            messagebox.showinfo("Success", self.get_ui_text("editor.msgSaved", "File saved successfully!"))
        except Exception as e:
            messagebox.showerror("Error", f"{self.get_ui_text('editor.errSave', 'Failed to save JSON:')}\n{e}")

    def on_search_change(self, *args):
        query = self.search_var.get().lower()
        self._search_results = []
        self._search_idx = -1
        
        if not query:
            self.lbl_search_count.config(text="0/0")
            self._build_search_results("", "")
            return
            
        self._build_search_results("", query)
        if self._search_results:
            self._search_idx = 0
            self._update_search_ui()
        else:
            self.lbl_search_count.config(text="0/0")

    def _update_search_ui(self):
        if not self._search_results: return
        self.lbl_search_count.config(text=f"{self._search_idx + 1}/{len(self._search_results)}")
        match_row = self._search_results[self._search_idx]
        
        parent = self.json_tree.parent(match_row)
        while parent:
            self.json_tree.item(parent, open=True)
            parent = self.json_tree.parent(parent)
            
        self.json_tree.selection_set(match_row)
        self.json_tree.see(match_row)
        self.selected_json_node = match_row

    def find_next(self):
        if not self._search_results: return
        self._search_idx = (self._search_idx + 1) % len(self._search_results)
        self._update_search_ui()
        
    def find_prev(self):
        if not self._search_results: return
        self._search_idx = (self._search_idx - 1) % len(self._search_results)
        self._update_search_ui()

    def _build_search_results(self, parent, query):
        for child in self.json_tree.get_children(parent):
            tags = list(self.json_tree.item(child, "tags"))
            if "search_match" in tags: tags.remove("search_match")
                
            if query:
                text = str(self.json_tree.item(child, "text")).lower()
                values = self.json_tree.item(child, "values")
                val_text = str(values[0]).lower() if values else ""
                raw_text = str(values[2]).lower() if values and len(values) >= 3 else ""
                
                if query in text or query in val_text or query in raw_text:
                    self._search_results.append(child)
                    tags.append("search_match")
            
            self.json_tree.item(child, tags=tags)
            self._build_search_results(child, query)

    def render_json_tree(self):
        self.json_tree.delete(*self.json_tree.get_children())
        root_node = self.insert_json_node("", "root", self.current_data, depth=0)
        if root_node: self.json_tree.item(root_node, open=True)
        
    def insert_json_node(self, parent, key, value, depth=0):
        is_open = depth < 3
        if isinstance(value, dict):
            summary = f"{{...}} ({len(value)} items)"
            node = self.json_tree.insert(parent, "end", text=str(key), values=(summary, "dict", ""), open=is_open)
            for k, v in value.items(): self.insert_json_node(node, k, v, depth + 1)
            return node
        elif isinstance(value, list):
            summary = f"[...] ({len(value)} items)"
            node = self.json_tree.insert(parent, "end", text=str(key), values=(summary, "list", ""), open=is_open)
            for i, v in enumerate(value): self.insert_json_node(node, i, v, depth + 1)
            return node
        else:
            val_type = type(value).__name__
            if value is None: val_type = "null"
            if isinstance(value, bool): val_type = "bool"
            
            display_val = str(value)
            if val_type == "str" and value in self.preview_i18n_data:
                display_val = self.preview_i18n_data[value]
            
            return self.json_tree.insert(parent, "end", text=str(key), values=(display_val, val_type, str(value)))

    def on_json_hover(self, event):
        item = self.json_tree.identify_row(event.y)
        if item:
            if item != self.last_hovered_item:
                self.tooltip.hidetip()
                self.last_hovered_item = item
                values = self.json_tree.item(item, "values")
                if values and len(values) >= 3:
                    raw_val = values[2]
                    display_val = values[0]
                    if raw_val != display_val:
                        self.after(400, self._show_tooltip_delayed, item, f"Key: {raw_val}")
        else:
            self.tooltip.hidetip()
            self.last_hovered_item = None
            
    def _show_tooltip_delayed(self, item, text):
        if self.last_hovered_item == item: self.tooltip.showtip(text)

    def on_json_leave(self, event):
        self.tooltip.hidetip()
        self.last_hovered_item = None

    def on_json_click(self, event):
        item = self.json_tree.selection()
        if item: self.selected_json_node = item[0]

    def on_json_double_click(self, event):
        row = self.json_tree.identify_row(event.y)
        col = self.json_tree.identify_column(event.x)
        if not row: return
        
        values = self.json_tree.item(row, "values")
        key_text = self.json_tree.item(row, "text")
        if not values: return
        
        val_type = values[1]
        raw_val = values[2] if len(values) >= 3 else values[0]
        
        if col == "#0":
            parent = self.json_tree.parent(row)
            if parent == "": return
            parent_type = self.json_tree.item(parent, "values")[1] if parent else "dict"
            if parent_type == "list": return
            self._spawn_inline_editor(row, col, key_text, "key")
            
        elif col == "#1":
            if val_type in ["dict", "list"]: return
            self._spawn_inline_editor(row, col, raw_val, "value", val_type)
            
        elif col == "#2":
            if val_type in ["dict", "list"]: return
            self._spawn_inline_combobox(row, col, val_type)

    def _spawn_inline_editor(self, row, col, current_val, edit_mode, val_type=None):
        x, y, w, h = self.json_tree.bbox(row, col)
        
        if edit_mode == "value" and val_type == "str":
            _, existing_strings = self._get_all_keys_and_strings()
            completion_dict = {s: s for s in existing_strings}
            for k, v in self.preview_i18n_data.items():
                display_str = f"{v} [{k}]"
                completion_dict[display_str] = k
            entry = AutocompleteEntry(self.json_tree, completion_dict)
        elif edit_mode == "key":
            existing_keys, _ = self._get_all_keys_and_strings()
            completion_dict = {k: k for k in existing_keys}
            entry = AutocompleteEntry(self.json_tree, completion_dict)
        else:
            entry = ttk.Entry(self.json_tree)
            
        entry.place(x=x, y=y, width=w, height=h)
        if isinstance(entry, AutocompleteEntry):
            if edit_mode == "value" and current_val in self.preview_i18n_data:
                display_str = f"{self.preview_i18n_data[current_val]} [{current_val}]"
                entry.set(display_str)
            else:
                entry.set(str(current_val))
        else:
            entry.insert(0, str(current_val))
            
        entry.select_range(0, tk.END)
        entry.focus_set()
        
        def save_edit(event=None):
            if not entry.winfo_exists(): return
            if isinstance(entry, AutocompleteEntry):
                new_val_str = entry.get_actual_value()
            else:
                new_val_str = entry.get()
            entry.destroy()
            self._apply_inline_edit(row, edit_mode, new_val_str, val_type)
            
        def safe_focus_out(event=None):
            if not entry.winfo_exists(): return
            fw = entry.focus_get()
            if fw and "popdown" in fw.winfo_name(): return
            save_edit()

        entry.bind("<Return>", save_edit)
        entry.bind("<Escape>", lambda e: entry.destroy())
        
        if isinstance(entry, AutocompleteEntry):
            entry.bind("<<ComboboxSelected>>", lambda e: entry.after(50, save_edit))
            entry.bind("<FocusOut>", lambda e: entry.after(150, safe_focus_out))
        else:
            entry.bind("<FocusOut>", save_edit)

    def _spawn_inline_combobox(self, row, col, current_type):
        x, y, w, h = self.json_tree.bbox(row, col)
        cb = ttk.Combobox(self.json_tree, values=["str", "int", "float", "bool", "null"], state="readonly")
        cb.place(x=x, y=y, width=w, height=h)
        cb.set(current_type)
        cb.focus_set()
        
        def save_type(event=None):
            if not cb.winfo_exists(): return
            new_type = cb.get()
            cb.destroy()
            self._apply_inline_type_edit(row, new_type)
            
        def safe_focus_out(event=None):
            if not cb.winfo_exists(): return
            fw = cb.focus_get()
            if fw and "popdown" in fw.winfo_name(): return
            save_type()
            
        cb.bind("<Return>", save_type)
        cb.bind("<<ComboboxSelected>>", lambda e: cb.after(50, save_type))
        cb.bind("<Escape>", lambda e: cb.destroy())
        cb.bind("<FocusOut>", lambda e: cb.after(150, safe_focus_out))

    def _apply_inline_edit(self, row, edit_mode, new_val_str, val_type):
        path = self._get_node_path(row)
        ref = self._get_data_ref(path[:-1]) if len(path) > 1 else self.current_data
        
        if edit_mode == "value":
            try:
                if val_type == "int": new_val = int(new_val_str)
                elif val_type == "float": new_val = float(new_val_str)
                elif val_type == "bool": 
                    if new_val_str.lower() in ["true", "1", "yes"]: new_val = True
                    elif new_val_str.lower() in ["false", "0", "no"]: new_val = False
                    else: raise ValueError("Invalid bool")
                elif val_type == "null": new_val = None
                else: new_val = new_val_str
            except ValueError:
                messagebox.showerror("Error", self.get_ui_text("editor.errInvalidType", "Invalid value for selected type."))
                return
                
            ref[path[-1]] = new_val
            display_val = str(new_val)
            if val_type == "str" and new_val in self.preview_i18n_data:
                display_val = self.preview_i18n_data[new_val]
            self.json_tree.item(row, values=(display_val, val_type, str(new_val)))
            
        elif edit_mode == "key":
            old_key = path[-1]
            new_key = new_val_str
            if old_key == new_key: return
            if new_key in ref:
                messagebox.showerror("Error", self.get_ui_text("editor.errKeyExists", "Key already exists"))
                return
                
            items = list(ref.items())
            ref.clear()
            for k, v in items:
                if k == old_key: ref[new_key] = v
                else: ref[k] = v
            self.json_tree.item(row, text=new_key)

    def _apply_inline_type_edit(self, row, new_type):
        path = self._get_node_path(row)
        values = self.json_tree.item(row, "values")
        raw_val = values[2] if len(values) >= 3 else values[0]
        
        try:
            if new_type == "int": new_val = int(raw_val)
            elif new_type == "float": new_val = float(raw_val)
            elif new_type == "bool": 
                if str(raw_val).lower() in ["true", "1", "yes"]: new_val = True
                elif str(raw_val).lower() in ["false", "0", "no"]: new_val = False
                else: new_val = bool(raw_val)
            elif new_type == "null": new_val = None
            else: new_val = str(raw_val)
        except ValueError:
            if new_type == "int": new_val = 0
            elif new_type == "float": new_val = 0.0
            elif new_type == "bool": new_val = False
            elif new_type == "null": new_val = None
            else: new_val = ""
            
        ref = self._get_data_ref(path[:-1]) if len(path) > 1 else self.current_data
        ref[path[-1]] = new_val
        
        display_val = str(new_val)
        if new_type == "str" and new_val in self.preview_i18n_data:
            display_val = self.preview_i18n_data[new_val]
        self.json_tree.item(row, values=(display_val, new_type, str(new_val)))

    def post_context_menu(self, event):
        item = self.json_tree.identify_row(event.y)
        if item:
            self.json_tree.selection_set(item)
            self.selected_json_node = item
            self.context_menu.post(event.x_root, event.y_root)

    def add_child(self, new_type):
        if not self.selected_json_node: return
        node = self.selected_json_node
        val_type = self.json_tree.item(node, "values")[1]
        
        if val_type not in ["dict", "list"]:
            messagebox.showerror("Error", self.get_ui_text("editor.errAddChild", "Can only add children to dict or list"))
            return
            
        path = self._get_node_path(node)
        ref = self._get_data_ref(path)
        
        default_val = {} if new_type == "dict" else ([] if new_type == "list" else "")
        
        if val_type == "dict":
            new_key = simpledialog.askstring("New Key", self.get_ui_text("editor.promptNewKey", "Enter new key:"))
            if not new_key: return
            if new_key in ref:
                messagebox.showerror("Error", self.get_ui_text("editor.errKeyExists", "Key already exists"))
                return
            ref[new_key] = default_val
            new_node = self.insert_json_node(node, new_key, default_val)
        else:
            ref.append(default_val)
            new_node = self.insert_json_node(node, len(ref)-1, default_val)
            
        self.json_tree.item(node, open=True)
        self.json_tree.selection_set(new_node)
        self.json_tree.see(new_node)

    def duplicate_item(self):
        if not self.selected_json_node: return
        node = self.selected_json_node
        parent = self.json_tree.parent(node)
        if not parent: return
        
        parent_type = self.json_tree.item(parent, "values")[1]
        if parent_type == "list":
            path = self._get_node_path(node)
            idx = path[-1]
            parent_path = path[:-1]
            ref = self._get_data_ref(parent_path)
            
            new_val = copy.deepcopy(ref[idx])
            ref.insert(idx + 1, new_val)
            
            expanded_nodes = self._get_expanded_nodes("", [])
            self.render_json_tree()
            self._restore_expanded_nodes("", expanded_nodes)
        else:
            messagebox.showinfo("Info", self.get_ui_text("editor.infoDuplicate", "Duplicate is only supported for list items."))

    def delete_item(self):
        if not self.selected_json_node: return
        node = self.selected_json_node
        parent = self.json_tree.parent(node)
        if not parent:
            messagebox.showerror("Error", self.get_ui_text("editor.errRootDelete", "Cannot delete root node"))
            return
            
        if messagebox.askyesno("Confirm", self.get_ui_text("editor.confirmDelete", "Are you sure you want to delete this item?")):
            path = self._get_node_path(node)
            key_or_idx = path[-1]
            parent_path = path[:-1]
            ref = self._get_data_ref(parent_path)
            
            if isinstance(ref, list):
                del ref[key_or_idx]
                expanded_nodes = self._get_expanded_nodes("", [])
                self.render_json_tree()
                self._restore_expanded_nodes("", expanded_nodes)
            elif isinstance(ref, dict):
                del ref[key_or_idx]
                self.json_tree.delete(node)
            self.selected_json_node = None

    def _get_node_path(self, node):
        path = []
        curr = node
        while curr:
            parent = self.json_tree.parent(curr)
            key = self.json_tree.item(curr, "text")
            if parent:
                parent_type = self.json_tree.item(parent, "values")[1]
                if parent_type == "list": key = int(key)
            if key != "root": path.insert(0, key)
            curr = parent
        return path

    def _get_data_ref(self, path):
        ref = self.current_data
        for p in path: ref = ref[p]
        return ref

    def _get_all_keys_and_strings(self):
        keys = set()
        strings = set()
        def extract(data):
            if isinstance(data, dict):
                for k, v in data.items():
                    keys.add(k)
                    extract(v)
            elif isinstance(data, list):
                for v in data:
                    extract(v)
            elif isinstance(data, str):
                strings.add(data)
        
        if self.current_data:
            extract(self.current_data)
        return keys, strings

    def _get_expanded_nodes(self, parent, expanded_list):
        for child in self.json_tree.get_children(parent):
            if self.json_tree.item(child, "open"):
                expanded_list.append(self._get_node_path(child))
            self._get_expanded_nodes(child, expanded_list)
        return expanded_list

    def _restore_expanded_nodes(self, parent, expanded_list):
        for child in self.json_tree.get_children(parent):
            if self._get_node_path(child) in expanded_list:
                self.json_tree.item(child, open=True)
            self._restore_expanded_nodes(child, expanded_list)
            
    def _restore_selected_node(self, parent, selected_path):
        for child in self.json_tree.get_children(parent):
            if self._get_node_path(child) == selected_path:
                self.json_tree.selection_set(child)
                self.json_tree.see(child)
                self.selected_json_node = child
                return True
            if self._restore_selected_node(child, selected_path):
                return True
        return False

if __name__ == "__main__":
    app = JSONEditorApp()
    app.mainloop()