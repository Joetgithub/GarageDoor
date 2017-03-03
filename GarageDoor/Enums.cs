using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace GarageDoor
{
    [Flags]
    public enum StatusBits
    {
        //Lifetime of sensor bit is untill the next check of the sensor occurs
        //sensor bits 1-8                        0000 0000
        Moving = 1,               //0                 0001
        DoorOpened = 2,           //1                 0010
        DoorClosed = 4,           //2                 0100
        Opening = 8,              //3                 1000
        Closing = 16,             //4               1 0000
        //Errors are cleared out after every command response
        //error bits  9-16             0000 0000
        ErrGlobalState = 256,     //5          1 0000 0000
        ErrLastOperation = 512,   //6         10 0000 0000
        ErrMoveTimeout = 1024,    //7        100 0000 0000
        ErrCannotStopMove = 2056, //8       1000 0000 0000
        ErrMoveNotStart = 4096,   //9     1 0000 0000 0000
        ErrWrongDirection = 8192, //10   10 0000 0000 0000
    };

    public enum DoorCommands
    {
        none = 0,
        pressButton = 1,
        awaitMoveComplete = 2,
        getStatus = 3,
    }

    public enum ParticleVariables
    {
        sensorLast = 1,
        sensorSaved = 2,
        status = 3,
        sensorBurst = 4,
        moveReadings = 5,
    }
}