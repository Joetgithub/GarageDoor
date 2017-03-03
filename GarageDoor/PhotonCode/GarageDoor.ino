#include <string>
#include <time.h>
SerialLogHandler logHandler;

//This enum is the backbone of the application.  Results of a particle command are
//passed back with an int. Setting individual bits of this int to provide results
//of a command back to caller as well as to keep track of the status of the application.
//This enum needs to be kept in sync with version of the enum on the client and server
enum StatusBits
{
	//Lifetime of sensor bit is untill the next check of the sensor occurs
	//sensor bits 1-8                              0000 0000
	bitMoving             = 0,   //1                    0001
    bitDoorOpened         = 1,   //2                    0010
    bitDoorClosed         = 2,   //4                    0100
    bitOpening            = 3,   //8                    1000
    bitClosing            = 4,   //16                 1 0000
	//Errors are cleared out after every command response
    //error bits  9-16                   0000 0000
    bitErrGlobalState     = 8,   //256           1 0000 0000
    bitErrLastOperation   = 9,   //512          10 0000 0000
    bitErrMoveTimeout     = 10,  //1024        100 0000 0000
    bitErrCannotStopMove  = 11,  //2056       1000 0000 0000
    bitErrMoveNotStart    = 12,  //4096     1 0000 0000 0000
	bitErrWrongDirection  = 13,  //8192    10 0000 0000 0000
};

//Just a away to try and organize better the validations that are occuring
enum TransitionTypes
{
	stoppedToMoving			= 1,
	movingToStopped			= 2,
	awaitingMoveCompletion 	= 3,
};
//method with TransitionTypes parameter needed early declare (or it won't compile)
void  validateStateTransitions(int oldStatus, TransitionTypes transitionType);

const int PIN_LED        	= D7;
const int PIN_HALL_A        = D6;	//green
const int PIN_HALL_B        = D4;	//white
const int PIN_HALL_C        = A2;   //brown using analog pin because of seeming conflict with D1
const int PIN_REED_BOTTOM   = D2;	//brown
const int PIN_RELAY         = D3;	//gray
const int PIN_REED_TOP      = D5;   //blue

const int MAX_DOORMOVE_TIME = 18;	//max time it should take for a door open/close in seconds

// Max number of timers allowed is 10
Timer timerPollDoorSensors(100, onTimerPollDoorSensors);				//reads door open/closed sensors
Timer timerPollRotateDirection(50, onTimerPollRotateDirection);         //detect direction of motion
Timer timerPrintSensors(500, onTimerPrintSensors);						//how often sensor readings are displayed to console

// 'Hold' onto a detected move for a specified time. A timer is used to check the elapsed time since
// the last move occured and if the elapsed time exceeds the specified time then the movement bit
// is turned off.  The hold time needs to be slightly greater than the maximum rotation time to
// maintain a constant movement condition. If rotating shaft has fixed RPM of 60 then delay needs to be > 1000ms.
const int MOVE_HOLDTIME = 500;										// time it takes for rotation to spin down
Timer timerMoveHoldRelease(MOVE_HOLDTIME, onTimerMoveHoldRelease);	// timer start triggered by IRQ
unsigned long _lastMoveTime = 0;	                                 // time of most recent move occurance

//Particle Variables -  "_pv" prefix indicates a Particle Variable
int _pvStatus = 0;				  // holds all bits for tracking application state
String _pvSensorLast;			  // continously updated after each sensor reading providing status of all sensors
String _pvSensorSaved;			  // saved sensor reading at the completion the last DoorCommand

//Below is used in mechanism for capturing and publishing activity that occurs while movement is happening
const bool PUBLISH_MOVEMENT_READINGS = true; //enables the collection and publishing of sensor data while movement occurs
const String MOVEREADINGS = "moveReadings";  //particle variable name that is published
const std::string PUBLISH_DATAREADY  = "ProdPublishDataReady";	//name of webook - need to toggle between "Test" or "Prod"
std::string _sensorMoveReadings;  // accumulates multiple sensor readings taken during a door open/close
String _pvLastSensorMoveReadings; // particle variable that holds json that includes the _sensorMoveReadings
Timer timerWhileDoorMoving(500, onTimerWhileDoorMoving);	  	//timer is only active when door is moving

//Below is used mechanism that determines direction of rotation
char _rotateOrder[4] = "";     	       // size is 1+ for null at end?
volatile int _rotateCount = 0;         // used in determining rotation direction
volatile int _revsPerRevInterval = 0;
unsigned long _lastRevReadTime = 0;

//Below is used in mechanism for determing rpm
int _revsPerMinute = 0;
const int REV_READ_INTERVAL = 500;               //frequency for reading of accumlated revs  in ms
Timer timerRevCounter(500, onTimerRevCounter);   //tracks rotations per a given time interval

void setup()
{
	Serial.begin(9600);

	pinMode(PIN_HALL_A, INPUT_PULLUP);
	pinMode(PIN_HALL_B, INPUT_PULLUP);
	pinMode(PIN_HALL_C, INPUT_PULLUP);
	pinMode(PIN_REED_TOP, INPUT_PULLUP);	 //normal state of this reed switch is HIGH
	pinMode(PIN_REED_BOTTOM, INPUT_PULLUP);  //...switch goes to LOW when it's triggered by magnet

	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_RELAY, OUTPUT);

	enableMovementDirectionDetectionMode();

    //Up to 15 cloud functions may be registered and each function
    //name is limited to a maximum of 12 characters.
    //Called with POST /v1/devices/{DEVICE_ID}/{FUNCTION}. Returns an int.
  	//Particle.function("DoorCommand", DoorCommand);
	Particle.function("DoorCommand", DoorCommand);

    //Up to 20 cloud variables may be registered and each variable name
    //is limited to a maximum of 12 characters.  Able to get this value directly
    //Get a meaningless compile error if go over 12 characters!
    //Called with GET /v1/devices/{DEVICE_ID}/{VARIABLE}.
	Particle.variable("sensorLast", _pvSensorLast);       //variable is HTTP GET
	Particle.variable("sensorSaved", _pvSensorSaved);
	Particle.variable("status", _pvStatus);
	Particle.variable(MOVEREADINGS, _pvLastSensorMoveReadings);

	readDoorSensors();

    timerPrintSensors.start();
	timerPollDoorSensors.start();
	timerMoveHoldRelease.start();
	timerPollRotateDirection.start();
}

void loop()		//main loop is on same thread as particle functions!
{
	//Offloading work that could have been done an interrupt to the main loop
	if (isBitOn(_pvStatus, bitMoving))
	{
		//update string holding current state of sensors whenever a change occurs
		_pvSensorLast = GetAllReadings();
		digitalWrite(PIN_LED, HIGH);
	}
}

// holds the 'before' state to compare to the 'after' state for state transition validation
int _beginStatus = 0;

int DoorCommand(String command)
{
	checkValidGlobalState();

	if (isBitOn(_pvStatus, bitErrGlobalState))
	{
		Log.info("DoorCommand: %s door is in an invalid state  %d", command.c_str(), _pvStatus);
		return returnDoorCommandResults();
	}

	// Dependancies exist between "pressButton" and "awaitMoveComplete"
	//  1) "pressButton" has to set _beginStatus for use by  "awaitMoveComplete"
	//  2) "pressButton" starts up timerWhileDoorMoving timer which is the turned off by "awaitMoveComplete"
	if (command == "pressButton")
	{
		_beginStatus = _pvStatus;
		Log.info("DoorCommand: %s start %d", command.c_str(), _pvStatus);
		if (isStopped())
		{
			Log.info("DoorCommand: %s door is not moving -- attempting to move %d", command.c_str(), _pvStatus);
			pressButton();	//first attempt to 'press' the button

			//handle the case where need to press the button 2X before door actually moves
			if (!(isBitOn(_pvStatus, bitMoving)))
			{
				pressButton();
				if (!isBitOn(_pvStatus, bitMoving))
				{
					Log.error("DoorCommand: %s door failed to start moving after two attempts %d", command.c_str(), _pvStatus);
					setBitOn(_pvStatus, bitErrMoveNotStart);
				}
			}

			// Begin timer that populates the sensor readings while the movement is occuring
			_sensorMoveReadings = "";	    // initialize variable that timerWhileDoorMoving populates
			if (PUBLISH_MOVEMENT_READINGS)
			{
				timerWhileDoorMoving.start();
			}

			delay(500);   // wait for updates from polling of reed switches
			validateStateTransitions(_beginStatus, stoppedToMoving);
		    Log.info("DoorCommand: %s door move is valid=%s %d", command.c_str(), !isBitOn(_pvStatus, bitErrLastOperation) ? "true":"false", _pvStatus);
		}
		else
		{
			Log.info("DoorCommand: %s door is moving -- attempt to stop %d", command.c_str(), _pvStatus);
			pressButton();
			delay(MOVE_HOLDTIME);  // wait for rotation to spin down
			if (isBitOn(_pvStatus, bitMoving))
			{
				pressButton();
				delay(MOVE_HOLDTIME);
				if (isBitOn(_pvStatus, bitMoving))
				{
					Log.error("DoorCommand: %s cannot stop moving door %d", command.c_str(), _pvStatus);
					setBitOn(_pvStatus, bitErrCannotStopMove);
				}
			}
			validateStateTransitions(_beginStatus, movingToStopped);
			Log.info("DoorCommand: %s door move stopped is valid=%s %d", command.c_str(), !isBitOn(_pvStatus, bitErrLastOperation) ? "true":"false", _pvStatus);
		}
		_beginStatus = _pvStatus;	// this is significant, must be done to set up validation of "awaitMoveComplete"
		return returnDoorCommandResults();
	}
	else if (command == "awaitMoveComplete")	// this command must occur after a "pressButton" command
	{
		Log.info("DoorCommand: %s start %d", command.c_str(), _pvStatus);
		int startTime = Time.now();
		int elapsed = 0;
		while (isBitOn(_pvStatus, bitMoving))
		{
			delay(50);
			elapsed = Time.now() - startTime;
			if (elapsed > MAX_DOORMOVE_TIME) break;
		}

		// 1) Populate the sensor data that is also a particle variable
		// 2) Send notification that particle variable is now ready to be read
		if (PUBLISH_MOVEMENT_READINGS && timerWhileDoorMoving.isActive())
		{
			//_pvLastSensorMoveReadings = String::format("{\"op\":\"Burst%sing\", \"all\":[%s]}", command.c_str(), _sensorMoveReadings.c_str());
			_pvLastSensorMoveReadings = String::format("{\"all\":[%s]}", _sensorMoveReadings.c_str());
			timerWhileDoorMoving.stop();
			Particle.publish(PUBLISH_DATAREADY.c_str(),MOVEREADINGS);	//send data out to webhook
			Serial.println(_pvLastSensorMoveReadings);
		}

		if (elapsed > MAX_DOORMOVE_TIME)
		{
			Log.error("DoorCommand: %s door still moving after timeout limit %d", command.c_str(), _pvStatus);
			setBitOn(_pvStatus, bitErrMoveTimeout);
			onTimerPrintSensors();	 //don't wait for update from the polling -- update right now instead!
			}

		validateStateTransitions(_beginStatus, awaitingMoveCompletion);
		Log.info("DoorCommand: %s awaitMoveComplete is valid=%s %d", command.c_str(), !isBitOn(_pvStatus, bitErrLastOperation) ? "true":"false", _pvStatus);
		return returnDoorCommandResults();
	}
	else if (command == "getStatus")
	{
		return _pvStatus;
	}
	else
	{
		Log.error("DoorCommand: %s invalid command %d", command.c_str(), _pvStatus);
		return -1;
	}
}

void checkValidGlobalState()
{
	if (isBitOn(_pvStatus, bitDoorOpened) && isBitOn(_pvStatus, bitDoorClosed))
	{
		Log.error("checkValidGlobalState: door is both open and closed");
		setBitOn(_pvStatus, bitErrGlobalState);
		return;
	}
	if (isBitOn(_pvStatus, bitDoorOpened) && isBitOn(_pvStatus, bitMoving))
	{
		Log.error("checkValidGlobalState: door is both open and moving");
		setBitOn(_pvStatus, bitErrGlobalState);
		return;
	}
	if (isBitOn(_pvStatus, bitDoorClosed) && isBitOn(_pvStatus, bitMoving))
	{
		Log.error("checkValidGlobalState: door is both closed and moving");
		setBitOn(_pvStatus, bitErrGlobalState);
		return;
	}
	if (isBitOn(_pvStatus, bitClosing) && isBitOn(_pvStatus, bitOpening))
	{
		Log.error("checkValidGlobalState: door is both closing and opening");
		setBitOn(_pvStatus, bitErrGlobalState);
		return;
	}
	if (isBitOn(_pvStatus, bitMoving) && (!isBitOn(_pvStatus, bitOpening) && !isBitOn(_pvStatus, bitClosing)))
	{
		Log.error("checkValidGlobalState: door is moving but is does not have a direction");
		setBitOn(_pvStatus, bitErrGlobalState);
		return;
	}
	setBitOff(_pvStatus, bitErrGlobalState);
}

void validateStateTransitions(int oldStatus, TransitionTypes transitionType)
{
	Log.info("validateStateTransitions: transitionType %d, oldStatus:%d, current:%d", transitionType, oldStatus, _pvStatus);
	bool isValid = false;
	switch(transitionType)
	{
		case stoppedToMoving:
			if (isBitOn(oldStatus, bitDoorOpened) && isBitOn(_pvStatus, bitClosing))
			{
				isValid = true;
			}
			else if (isBitOn(oldStatus, bitDoorClosed) && isBitOn(_pvStatus, bitOpening))
			{
				isValid = true;
			}
			else if (isDoorPartiallyOpen(oldStatus) &&
				     (isBitOn(_pvStatus, bitOpening) || isBitOn(_pvStatus, bitClosing)))
			{
				isValid = true;
			}
			else if  (isBitOn(oldStatus, bitDoorOpened) && isBitOn(_pvStatus, bitOpening))
			{
				Log.error("validateStateTransitions: trying to open an allready opened door");
				isValid = false;
			}
			else if (isBitOn(oldStatus, bitDoorClosed) && isBitOn(_pvStatus, bitClosing))
			{
				Log.error("validateStateTransitions: trying to close  an allready closed door");
				isValid = false;
			}
			break;
		case movingToStopped:
			isValid = isStopped();
			break;
		case awaitingMoveCompletion:
			if (isBitOn(oldStatus, bitClosing) && isBitOn(_pvStatus, bitDoorClosed))
			{
				isValid = true;
			}
			else if (isBitOn(oldStatus, bitOpening) && isBitOn(_pvStatus, bitDoorOpened))
			{
				isValid = true;
			}
			else if ((isBitOn(oldStatus, bitClosing) && isBitOn(_pvStatus, bitDoorClosed)) ||
			         (isBitOn(oldStatus, bitOpening) && isBitOn(_pvStatus, bitDoorOpened)))
			{
				isValid = true;
			}
			else if ((isBitOn(oldStatus, bitClosing) && isBitOn(_pvStatus, bitDoorOpened)) ||
			         (isBitOn(oldStatus, bitOpening) && isBitOn(_pvStatus, bitDoorClosed)))
			 {
				 Log.error("Moving up caused door to be closed or move down caused door to be open");
				 setBitOn(_pvStatus, bitErrWrongDirection);
			 }
			break;
		default:
			Log.error("validateStateTransitions: invalid transitionType %d, status:%d", transitionType, _pvStatus);
			break;
	}
	if (!isValid) { setBitOn(_pvStatus, bitErrLastOperation); }
}

void pressButton()
{
	//Purpose of delays is to wait for sensor dections to occur
	digitalWrite(PIN_RELAY, HIGH); 	//push putton down
	delay(250);						//and keep it pressed down
	digitalWrite(PIN_RELAY, LOW);	//now release it
	delay(500);						//needs to be long enough for all sensor bits to bits to update
}

bool isStopped()
{
	if (isBitOn(_pvStatus, bitMoving))
	{
		return false;
	}
	return
		isBitOn(_pvStatus, bitDoorOpened) ||
		isBitOn(_pvStatus, bitDoorClosed) ||
		isDoorPartiallyOpen(_pvStatus);
}

bool isDoorPartiallyOpen(int status)
{
	return !isBitOn(status, bitDoorClosed) && !isBitOn(status, bitDoorOpened);
}

void enableMovementDirectionDetectionMode()
{
	// determine direction of rotation by order in which three sensors trigger
	detachInterrupt(PIN_HALL_A);
	attachInterrupt(PIN_HALL_A, irqHallTriggered_A1, FALLING);
	attachInterrupt(PIN_HALL_B, irqHallTriggered_B, FALLING);
	attachInterrupt(PIN_HALL_C, irqHallTriggered_C, FALLING);
}

void enableMovementDetectionMode()
{
	// 1) disable ability to detect the direction of rotation
	detachInterrupt(PIN_HALL_A);
	detachInterrupt(PIN_HALL_B);
	detachInterrupt(PIN_HALL_C);
	// 2) enable ability to detect if rotation is occuring or not
	attachInterrupt(PIN_HALL_A, irqHallMotionDetect, FALLING);
	// 3) ability to keep track speed of revolutions
	enableRevCounter();
}

void irqHallMotionDetect()
{
	// - No debouncing needed with this sensor!
    // - hold onto the detection of trigger for a little bit
	// - keeping logic in the interrupt to a minimum
	// - additional work is handled in the main loop when movement occurs
	setBitOn(_pvStatus, bitMoving);
	_lastMoveTime = millis();

	_revsPerRevInterval++;
}

void irqHallTriggered_A1()
{
    Log.info("%d A1 triggered", _rotateCount);
	//_lastMoveTime = millis();
	if (_rotateCount <= 2)
	{
		Log.info("\tA set");
		_rotateOrder[_rotateCount] = 'A';
		_rotateCount++;
	}
}

void irqHallTriggered_B()
{
    Log.info("%d B triggered", _rotateCount);
	if (_rotateCount <= 2)
	{
		Log.info("\tB set");
		_rotateOrder[_rotateCount] = 'B';
		_rotateCount++;
	}
}

void irqHallTriggered_C()
{
    Log.info("%d C triggered", _rotateCount);
	if (_rotateCount <= 2)
	{
		Log.info("\tC set");
		_rotateOrder[_rotateCount] = 'C';
		_rotateCount++;
	}
}

void onTimerRevCounter()
{
	if (millis() - _lastRevReadTime == REV_READ_INTERVAL)
	{
		_revsPerMinute = _revsPerRevInterval * (60000 / REV_READ_INTERVAL);
		Log.info("revs per interval:%d rpm:%d", _revsPerRevInterval, _revsPerMinute);
		_revsPerRevInterval = 0;
		_lastRevReadTime = millis();
	}
}

void onTimerMoveHoldRelease()
{
	unsigned long now = millis();
	//Serial.printlnf("onTimerMoveHoldRelease elapsed: %d",now - _lastMoveTime);
	if ((now - _lastMoveTime) > MOVE_HOLDTIME)
	{
		_lastMoveTime = 0;
		//Serial.printlnf("\trelease: %d", _lastMoveTime);

	    digitalWrite(PIN_LED, LOW);

		setBitOff(_pvStatus, bitMoving );
		disableRevCounter();

		setBitOff(_pvStatus, bitOpening );
		setBitOff(_pvStatus, bitClosing );
		enableMovementDirectionDetectionMode();

		//immediately update string holding current state of sensors
		_pvSensorLast = GetAllReadings();
	}
	else
	{
		_lastMoveTime = _lastMoveTime - (now - _lastMoveTime);
		//Serial.printlnf("\tholding: %d", _lastMoveTime);
	}
}

void onTimerPollDoorSensors(){
	readDoorSensors();
}

void onTimerPollRotateDirection()
{
	if (_rotateCount == 3)
    {
        String order = String(_rotateOrder);  // 4th position is null terminated so only getting 3 chars?
        if (order == "ABC" || order == "BCA" || order == "CAB")
        {
            Log.info("%s clockwise = opening", order.c_str());
			setBitOn(_pvStatus, bitOpening);
			//setBitOn(_pvStatus, bitClosing);  //for testing
			enableMovementDetectionMode();
        }
        else if (order == "CBA" || order == "BAC" || order == "ACB")
        {
            Log.info("%s counter clockwise = closing", order.c_str());
			setBitOn(_pvStatus, bitClosing);
			enableMovementDetectionMode();
        }
        else
        {
            Log.info("''%s' direction unknown", order.c_str());
        }
		// 0 is used as a flag to block direction detection logic here and in irqs
        _rotateCount = 0;
        memset(_rotateOrder, 0, sizeof _rotateOrder);	// reset array to 0's
    }
}

void onTimerPrintSensors()
{
	Log.info("status:%d %s", _pvStatus, GetAllReadings().c_str());
}

void onTimerWhileDoorMoving()
{
    _sensorMoveReadings.append(GetMovementReadings() + ",");
    Log.info("onTimerWhileDoorMoving");
}

String GetAllReadings()
{
	return  String::format(
		"{\"t\":\"%s\",\"o\":\"%d\",\"c\":\"%d\",\"m\":\"%d\",\"u\":\"%d\",\"d\":\"%d\",\"r\":\"%d\"}",
	 	Time.format(Time.now(),"%H:%M:%S").c_str(),
		isBitOn(_pvStatus, bitDoorOpened), isBitOn(_pvStatus, bitDoorClosed),
		isBitOn(_pvStatus, bitMoving),
		isBitOn(_pvStatus, bitOpening), isBitOn(_pvStatus, bitClosing), _revsPerMinute);
}

String GetMovementReadings()
{
	return  String::format(
		"{\"t\":\"%s\",\"r\":\"%d\"}",
	 	Time.format(Time.now(),"%H:%M:%S").c_str(), _revsPerMinute);
}
void readDoorSensors()
{
    //Looking for HIGH to LOW transition of normally open reed switch
    if (digitalRead(PIN_REED_TOP) == LOW)
    {
        setBitOn(_pvStatus, bitDoorOpened);
    }
    else
    {
        setBitOff(_pvStatus, bitDoorOpened);
    }

	//Looking for HIGH to LOW transition of normally open reed switch
    if (digitalRead(PIN_REED_BOTTOM) == LOW)
    {
        setBitOn(_pvStatus, bitDoorClosed);
    }
    else
    {
        setBitOff(_pvStatus, bitDoorClosed);
    }

	//update string holding current state of sensors whenever a change occurs
	_pvSensorLast = GetAllReadings();
}

void enableRevCounter()
{
	_revsPerRevInterval = 0;
	_revsPerMinute = 0;
	timerRevCounter.start();
	_lastRevReadTime = millis();
}

void disableRevCounter()
{
	timerRevCounter.stop();
}

//Helper method to do any extra work upon the completion of a DoorCommand
int returnDoorCommandResults()
{
	// keep the last reading
	_pvSensorSaved = _pvSensorLast;

	// clear peristing bits that provide the details on the execution of the DoorCommand
	int tempStatus = _pvStatus;
	//isn't there a quick 'mask' way to do all of these in one operation?
	setBitOff(_pvStatus, bitErrMoveTimeout);
	setBitOff(_pvStatus, bitErrWrongDirection);
	setBitOff(_pvStatus, bitErrMoveNotStart);
	setBitOff(_pvStatus, bitErrGlobalState);
	setBitOff(_pvStatus, bitErrCannotStopMove);
	setBitOff(_pvStatus, bitErrLastOperation);

	// return stattus of this execution of DoorCommand
	return tempStatus;
}

//http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c?answertab=oldest#tab-top
int isBitOn(int flagEnum, int bit)
{
    //To check a bit, shift the number x to the right, then bitwise AND it:
    return flagEnum >> bit & 1;
}

void setBitOn(int &flagEnum, int bit)
{
    //Use the bitwise OR operator (|) to set a bit.
    flagEnum |= 1 << bit;
}

void setBitOff(int &flagEnum, int bit)
{
    //Invert the bit string with the bitwise NOT operator (~), then AND it
    flagEnum &=~(1 << bit);
}
