#include "./includes/StateComposer.hpp"

// The I^2C serial bus device in the linux system
#define I2C_BUS "/dev/i2c-1"


// Required LED intensity scalar to use all 60 
// LEDs at full white, off of the Arduino 5V rail
// 5V rail can drive a maximum of 500mA
// 60 * .2 * .033 = .396A (396mA)

// We All Didn't Know Any Better Stupidity Inhibitor
// TODO: Update once better power system is found
#define WADKABSI 0.2

// The amount of microseconds to wait for the 
// Arduino Nano to send it's ACK and catch up
// last updated to: 12000
#define WAIT_ACK 12000


// Required for static class members
bool StateComposer::composeEnable;

// UART
//int StateComposer::uartFilestream;
//struct termios StateComposer::options;

// i2c
int StateComposer::i2cFileStream;

// Composer variables
bool StateComposer::logEnable;
std::ofstream StateComposer::logFile;
char StateComposer::composerState;

// Timing
time_t StateComposer::sysTime;
int StateComposer::weekDay;
unsigned int StateComposer::seconds;

// Internal State variables
Profile* StateComposer::currentProfile;
Schedule* StateComposer::currentZoneSchedule;
LEDState* StateComposer::currentZoneActiveState;

unsigned char StateComposer::red;
unsigned char StateComposer::green;
unsigned char StateComposer::blue;
int  StateComposer::intensity;
bool StateComposer::power;

std::vector<LED*> StateComposer::currentZoneLEDs;
Controller* StateComposer::currentLEDController;

unsigned char StateComposer::ioPort;
unsigned char StateComposer::stripIndex;


// Initialization
bool StateComposer::initialize(bool log)
{
    composeEnable = true;
    
    logEnable = log;
    composerState = 'C';

    tm* timeInfo;
    time(&sysTime);
    timeInfo = localtime(&sysTime);
    char timeBuffer[30];
    strftime(timeBuffer, 30, "%c", timeInfo);

    if (logEnable) {
        logFile.open("composer.log");
        logFile << "[" << timeBuffer << "] StateComposer transcript started\n";
    }

    i2cFileStream = open(I2C_BUS, O_RDWR | O_NOCTTY | O_NDELAY);  // Open in non terminal, non blocking, read-write mode

	if (i2cFileStream == -1) { // ERROR - CAN'T OPEN SERIAL PORT
		std::cerr << "ERROR: Unable to open i2c bus on " << I2C_BUS << "! \nEnsure it is not in use by another application.\n" << std::endl;
        return false;
	}

    // CONFIGURE THE UART
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)

    // tcgetattr(uartFilestream, &options);
	// options.c_cflag = B57600 | CS8 | CLOCAL | CREAD;		// Set baud rate, bits in-transmission, etc
	// options.c_iflag = IGNPAR;                               // Ignore parity
	// options.c_oflag = 0;
	// options.c_lflag = 0;
	// tcflush(uartFilestream, TCIFLUSH);
	// tcsetattr(uartFilestream, TCSANOW, &options);

    return true;
}



// Send and receive serial over uart w/ correct timings
bool StateComposer::serial_send(unsigned char io, unsigned char r, unsigned char g, unsigned char b, unsigned char idx)
{
	unsigned char s_buffer[4];
	unsigned char* p_s_buffer;
    char timeBuffer[30];

    tm* timeInfo;
    time(&sysTime);
    timeInfo=localtime(&sysTime);

    if (logEnable) {
        strftime(timeBuffer, 30, "%c", timeInfo);
        logFile << "[" << timeBuffer << "] "
                        << "Attempting i2c send:\n"
                        << "  Idx:" << (int)idx << "\n"
                        << "  R:" << (int)r << "\n"
                        << "  G:" << (int)g << "\n"
                        << "  B:" << (int)b << "\n";
    }

	p_s_buffer = &s_buffer[0]; // Reset pointer to head of array
	*p_s_buffer++ = idx;
    *p_s_buffer++ = r;
    *p_s_buffer++ = g;
    *p_s_buffer++ = b;
	
	if (i2cFileStream != -1) {

        // TODO: set third arg back to io rather than hard coded for testing
        if (ioctl(i2cFileStream, I2C_SLAVE, 7)) {   // Set io control for the i2c file stream, as sending to i2c slave, at address
            std::cerr << "ERROR: Can't access i2c bus address: " << std::hex << io << std::endl;
            logFile << "ERROR: Can't access i2c bus address: " << std::hex << io << std::endl;
            return true;    // Error state
        }


		int count = write(i2cFileStream, &s_buffer[0], (p_s_buffer - &s_buffer[0]));    // Filestream, bytes to write, number of bytes to write
		if (count < 0) {
			std::cerr << "ERROR: i2c transmit failed! (" << std::hex << io << ")" << std::endl;
            logFile << "ERROR: i2c transmit failed! (" << std::hex << io << ")" << std::endl;
            return true;    // Error state
		}
        
        usleep(WAIT_ACK); // Let Arduino catch up
	}

	return false;
}



// Main composer function
void StateComposer::compose()
{
    composerState = 'C';
    float scalar = 0.0;
    char timeBuffer[30];

    tm* timeInfo;
    time(&sysTime);
    timeInfo=localtime(&sysTime);

    weekDay = timeInfo->tm_wday;
    seconds = ( (timeInfo->tm_hour * 3600) + (timeInfo->tm_min * 60) + (timeInfo->tm_sec) );

    if (logEnable) {
        strftime(timeBuffer, 30, "%c", timeInfo);
        logFile << "[" << timeBuffer << "] Starting composition of internal state to hardware\n";
    }
    
    currentProfile = InternalState::get_current_profile();
    if (currentProfile == NULL) {
        if (logEnable) {
            logFile << "[" << timeBuffer << "] No profiles to loop on. Exiting composer\n";
        }
        return;
    }

    if (logEnable) {
        logFile << "[" << timeBuffer << "] Looping on active profile zones\n";
    }

    // Will only loop over returned vector of zones (if none, skip)
    for (auto currentZone : currentProfile->get_zones()) {

        currentZoneActiveState = currentZone->get_active_state(seconds, weekDay);
        if (currentZoneActiveState == NULL) {
            continue;
        }

        // Gather and compute color data
        intensity = currentZoneActiveState->get_intensity();
        power = currentZoneActiveState->get_power();

        scalar = (float)( ((float)intensity / 100.0) * (float)power * WADKABSI );

        if (logEnable)
            logFile << "[" << timeBuffer << "] Power Scalar for current zone: " << scalar << "\n";

        red = ((unsigned char) ( ((float)currentZoneActiveState->get_r()) * scalar)); 
        green = ((unsigned char) ( ((float)currentZoneActiveState->get_g()) * scalar));
        blue = ((unsigned char) ( ((float)currentZoneActiveState->get_b()) * scalar));

        currentZoneLEDs = currentZone->get_leds();

        if (logEnable) {
            logFile << "[" << timeBuffer << "] Looping on zone '" << currentZone->get_name() << "' LEDs\n";
        }

        // Will only loop over returned vector of LEDs (if none, skip)
        for (auto currentLED : currentZoneLEDs) {
        
            // Get controller info
            currentLEDController = currentLED->get_controller();
            if (currentLEDController == NULL) {
                continue;
            }

            ioPort = (unsigned char)currentLEDController->get_io();
            if (ioPort <= 0) {
                continue;
            }

            // Get LED index
            stripIndex = (unsigned char)currentLED->get_strip_idx();
            if (stripIndex < 0) {
                continue;
            }

            // Call serial send
            composerState = 'S';

            if (serial_send(ioPort, red, green, blue, stripIndex)) {
                logFile << "[" << timeBuffer << "] " << "Error transmitting serial!\n";
            }
            composerState = 'C';
            // END OF LEDS LOOP
        }

        // END OF ZONES LOOP
    }

    // END OF CURRENT STATE ORCHESTRATION
}


void StateComposer::led_shutdown()
{
    char timeBuffer[30];

    tm* timeInfo;
    time(&sysTime);
    timeInfo=localtime(&sysTime);
    strftime(timeBuffer, 30, "%c", timeInfo);

    for (auto currentLED : InternalState::get_leds())
    {
        if (currentLED) {

            // Get controller info
            currentLEDController = currentLED->get_controller();
            if (currentLEDController == NULL) {
                continue;
            }

            // Controller IO (deviceID)
            ioPort = (unsigned char)currentLEDController->get_io();
            if (ioPort <= 0) {
                continue;
            }

            // Get LED index
            stripIndex = (unsigned char)currentLED->get_strip_idx();
            if (stripIndex < 0) {
                continue;
            }

            // Call serial send
            if (serial_send(ioPort, '\0', '\0', '\0', stripIndex)) {
                logFile << "[" << timeBuffer << "] " << "Error transmitting serial for led_shutdown!\n";
            }
        }
    }
}


void StateComposer::clean_up() 
{
    if (logEnable)
        logFile.close();

    close(i2cFileStream);
}


char StateComposer::get_composer_state() 
{
    return composerState;
}