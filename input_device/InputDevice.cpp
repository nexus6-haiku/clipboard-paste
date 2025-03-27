/*
 * Copyrights (c):
 *     2001 - 2008 , Werner Freytag.
 *     2009, Haiku
 * Distributed under the terms of the MIT License.
 *
 * Original Author:
 *              Werner Freytag <freytag@gmx.de>
 *
 * Minimal changes by Humdinger, humdingerb@gmail.com
 */

#include "InputDevice.h"

#include <InterfaceDefs.h>

#include <stdlib.h>
#include <syslog.h>


thread_id AutoPasteInputDevice::fThread = B_ERROR;
thread_id AutoPasteInputDevice::fPort = B_ERROR;


BInputServerDevice* instantiate_input_device()
{
	return (new AutoPasteInputDevice());
}


AutoPasteInputDevice::AutoPasteInputDevice()
	:
	BInputServerDevice()
{
	static input_device_ref AutoPasteDevice = {
		(char *)"AutoPaste input device", B_UNDEFINED_DEVICE, NULL};
	static input_device_ref *AutoPasteDeviceList[2] = {
		&AutoPasteDevice, NULL};

	RegisterDevices(AutoPasteDeviceList);

//	char *ident = "AutoPaste device";
//	int logopt = LOG_PID | LOG_CONS;
//	int facility = LOG_USER;
//	openlog(ident, logopt, facility);
//	syslog(LOG_INFO, "Init AutoPaste device");
}


AutoPasteInputDevice::~AutoPasteInputDevice()
{
	// cleanup
}


status_t
AutoPasteInputDevice::InitCheck()
{
	// do any init code that could fail here
	// you will be unloaded if you return false

	return (BInputServerDevice::InitCheck());
}


status_t
AutoPasteInputDevice::SystemShuttingDown()
{
	// do any cleanup (ie. saving a settings file) when the
	// system is about to shut down

	return (BInputServerDevice::SystemShuttingDown());
}


status_t
AutoPasteInputDevice::Start(const char* device, void* cookie)
{
	// start generating events
	// this is a hook function, it is called for you
	// (you should not call it yourself)

	fThread = spawn_thread(listener, device, B_LOW_PRIORITY, this);

	resume_thread(fThread);
	return B_NO_ERROR;
}


status_t
AutoPasteInputDevice::Stop(const char* device, void* cookie)
{
	// stop generating events
	// this is a hook function, it is called for you
	// (you should not call it yourself)

	delete_port(fPort);
	fPort = B_ERROR;

	status_t err = B_OK;
	wait_for_thread(fThread, &err);
	fThread = B_ERROR;
	return err;
}


status_t
AutoPasteInputDevice::Control(const char* device, void* cookie,
	uint32 code, BMessage* message)
{
	return B_NO_ERROR;
}


int32
AutoPasteInputDevice::listener(void* arg)
{
	fPort = create_port(20, "AutoPaste output port");

	AutoPasteInputDevice* AutoPasteDevice = (AutoPasteInputDevice *)arg;

	int32 code;
	while (read_port(fPort, &code, NULL, 0) == B_OK) {

		BMessage* event = new BMessage(B_KEY_DOWN);
		event->AddInt64("when", system_time());
		event->AddInt32("key", 79);
		event->AddInt32("modifiers", B_COMMAND_KEY);
		event->AddInt8("states", 0);
		event->AddInt8("byte", 'v');
		event->AddString("bytes", "v");
		event->AddInt32("raw_char", 118);

		AutoPasteDevice->EnqueueMessage(event);

		// syslog(LOG_INFO, "AutoPaste device: Added event");

		snooze(100000);
	}

	return B_OK;
}
