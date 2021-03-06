/* Copyright (c) 2010-2012 Benjamin Dobell, Glass Echidna
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.*/

// C Standard Library
#include <stdio.h>

// Heimdall
#include "Arguments.h"
#include "BridgeManager.h"
#include "DownloadPitAction.h"
#include "Heimdall.h"
#include "Interface.h"

using namespace Heimdall;

const char *DownloadPitAction::usage = "Action: download-pit\n\
Arguments: --output <filename> [--verbose] [--no-reboot] [--stdout-errors]\n\
    [--delay <ms>]\n\
Description: Downloads the connected device's PIT file to the specified\n\
    output file.\n";

int DownloadPitAction::Execute(int argc, char **argv)
{
	// Handle arguments

	map<string, ArgumentType> argumentTypes;
	argumentTypes["output"] = kArgumentTypeString;
	argumentTypes["no-reboot"] = kArgumentTypeFlag;
	argumentTypes["delay"] = kArgumentTypeUnsignedInteger;
	argumentTypes["verbose"] = kArgumentTypeFlag;
	argumentTypes["stdout-errors"] = kArgumentTypeFlag;

	Arguments arguments(argumentTypes);

	if (!arguments.ParseArguments(argc, argv, 2))
	{
		Interface::Print(DownloadPitAction::usage);
		return (0);
	}

	const StringArgument *outputArgument = static_cast<const StringArgument *>(arguments.GetArgument("output"));

	if (!outputArgument)
	{
		Interface::Print("Output file was not specified.\n\n");
		Interface::Print(DownloadPitAction::usage);
		return (0);
	}

	const UnsignedIntegerArgument *communicationDelayArgument = static_cast<const UnsignedIntegerArgument *>(arguments.GetArgument("delay"));

	bool reboot = arguments.GetArgument("no-reboot") == nullptr;
	bool verbose = arguments.GetArgument("verbose") != nullptr;
	
	if (arguments.GetArgument("stdout-errors") != nullptr)
		Interface::SetStdoutErrors(true);

	// Info

	Interface::PrintReleaseInfo();
	Sleep(1000);

	// Open output file

	const char *outputFilename = outputArgument->GetValue().c_str();
	FILE *outputPitFile = fopen(outputFilename, "wb");

	if (!outputPitFile)
	{
		Interface::PrintError("Failed to open output file \"%s\"\n", outputFilename);
		return (1);
	}

	// Download PIT file from device.

	int communicationDelay = BridgeManager::kCommunicationDelayDefault;

	if (communicationDelayArgument)
		communicationDelay = communicationDelayArgument->GetValue();

	BridgeManager *bridgeManager = new BridgeManager(verbose, communicationDelay);

	if (bridgeManager->Initialise() != BridgeManager::kInitialiseSucceeded || !bridgeManager->BeginSession())
	{
		fclose(outputPitFile);
		delete bridgeManager;

		return (1);
	}

	unsigned char *pitBuffer;
	int fileSize = bridgeManager->DownloadPitFile(&pitBuffer);

	bool success = true;

	if (fileSize > 0)
	{
		if (fwrite(pitBuffer, 1, fileSize, outputPitFile) != fileSize)
		{
			Interface::PrintError("Failed to write PIT data to output file.\n");
			success = false;
		}
	}
	else
	{
		success = false;
	}

	if (!bridgeManager->EndSession(reboot))
		success = false;

	delete bridgeManager;
	
	fclose(outputPitFile);
	delete [] pitBuffer;

	return (success ? 0 : 1);
}
