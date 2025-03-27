/*
 * Copyright 2024
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Application.h>
#include <Clipboard.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <Message.h>
#include <InterfaceDefs.h>
#include <kernel/OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <storage/File.h>
#include <storage/FindDirectory.h>

#define OUTPUT_PORT_NAME "AutoPaste output port"

class AutoPasteApp : public BApplication {
public:
    AutoPasteApp();
    virtual void ArgvReceived(int32 argc, char** argv);
    virtual void ReadyToRun();

private:
    BMessage originalClipboard;
    BString textToPaste;
    bool textProvided;

    bool SaveCurrentClipboard();
    bool SetClipboardText(const char* text);
    bool TriggerPaste();
    bool RestoreClipboard();
};

AutoPasteApp::AutoPasteApp()
    : BApplication("application/x-vnd.AutoPaste"),
      textProvided(false)
{
}

void AutoPasteApp::ArgvReceived(int32 argc, char** argv)
{
    // Process command line arguments
    if (argc > 1) {
        // Use the first argument as the text to paste
        textToPaste = argv[1];
        textProvided = true;

        // If there are more arguments, concatenate them with spaces
        for (int32 i = 2; i < argc; i++) {
            textToPaste << " " << argv[i];
        }
    }
}

void AutoPasteApp::ReadyToRun()
{
    if (!textProvided) {
        printf("Usage: autopaste \"text to paste\"\n");
        PostMessage(B_QUIT_REQUESTED);
        return;
    }

    // Execute the sequence: save clipboard, set new clipboard text, trigger paste, restore clipboard
    if (SaveCurrentClipboard()) {
        if (SetClipboardText(textToPaste.String())) {
            if (TriggerPaste()) {
                RestoreClipboard();
            }
        }
    }

    // Quit the application when done
    PostMessage(B_QUIT_REQUESTED);
}

bool AutoPasteApp::SaveCurrentClipboard()
{
    printf("Saving current clipboard...\n");

    // Use BClipboard to get current clipboard content
    if (!be_clipboard->Lock()) {
        printf("Failed to lock clipboard\n");
        return false;
    }

    // Save the current clipboard content to our BMessage
    originalClipboard = *(be_clipboard->Data());

    be_clipboard->Unlock();
    return true;
}

bool AutoPasteApp::SetClipboardText(const char* text)
{
    printf("Setting clipboard text: %s\n", text);

    if (!be_clipboard->Lock()) {
        printf("Failed to lock clipboard for setting text\n");
        return false;
    }

    // Get the clipboard's data message
    BMessage* clip = be_clipboard->Data();

    // Replace existing text/plain data or add if it doesn't exist
    clip->RemoveName("text/plain");
    clip->AddData("text/plain", B_MIME_TYPE, text, strlen(text));

    status_t status = be_clipboard->Commit();
    be_clipboard->Unlock();

    if (status != B_OK) {
        printf("Failed to set clipboard text: %s\n", strerror(status));
        return false;
    }

    return true;
}

bool AutoPasteApp::TriggerPaste()
{
    printf("Triggering paste operation...\n");

    // Find the output port created by the input device
    port_id port = find_port(OUTPUT_PORT_NAME);
    if (port != B_NAME_NOT_FOUND) {
        // Send a message to the port to trigger Command+V
        write_port(port, 'CtSV', NULL, 0);
        printf("Paste message sent to input device\n");

        // Give the system a moment to process the paste operation
        snooze(500000); // 500ms
        return true;
    } else {
        printf("Error: AutoPaste output port not found\n");
        printf("Make sure the AutoPaste input device is running\n");
        return false;
    }
}

bool AutoPasteApp::RestoreClipboard()
{
    printf("Restoring original clipboard...\n");

    // Restore the original clipboard content
    if (!be_clipboard->Lock()) {
        printf("Failed to lock clipboard for restore\n");
        return false;
    }

    be_clipboard->Clear();

    // Copy our saved BMessage back to the clipboard
    BMessage* clip = be_clipboard->Data();
    *clip = originalClipboard;

    status_t status = be_clipboard->Commit();
    be_clipboard->Unlock();

    if (status != B_OK) {
        printf("Failed to restore clipboard: %s\n", strerror(status));
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    AutoPasteApp app;
    app.Run();
    return 0;
}
