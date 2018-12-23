#ifndef _LOCK
#define _LOCK

class Lock
{

private:
    bool obj;

public:
    Lock()
    {
        obj = false;
    }
    
    void lock()
    {
        while(obj);
        obj = true;
    }

    void unlock()
    {
        obj = false;
    }

};


#endif

