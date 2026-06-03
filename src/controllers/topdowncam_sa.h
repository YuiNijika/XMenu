#pragma once

/*
    TopDown for SA
*/
class TopDownCamera
{
private:
    TopDownCamera();
    TopDownCamera(const TopDownCamera&) = delete;
    
public:
    static TopDownCamera& Get() {
        static TopDownCamera instance;
        return instance;
    }

    void Process();
    void Disable();
};

extern TopDownCamera& TopDownCam;