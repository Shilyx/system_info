#include <Windows.h>
#include <tchar.h>
#include <intrin.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "globalVars.h"
#include "Convert.h"
#include "sysinfo.h"
#include "SystemInfo.h"
#include "utility.h"
#pragma comment(lib, "wbemuuid.lib")
//the actual function that does all the work
int getSystemInformation(SystemInfo *localMachine)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		return 1;                  // Program has failed.
	}

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
		);


	if (FAILED(hres))
	{
		cout << "Failed to initialize security. Error code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                    // Program has failed.
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object."
			<< " Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices *pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object 
		&pSvc                    // pointer to IWbemServices proxy
		);

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x"
			<< hex << hres << endl;
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

	cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
		);

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket. Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	getOS(localMachine, hres, pSvc, pLoc);
	getCPU(localMachine, hres, pSvc, pLoc);
	getMB(localMachine, hres, pSvc, pLoc);
	getRAM(localMachine, hres, pSvc, pLoc);
	getGPU(localMachine, hres, pSvc, pLoc);
	getMonitor(localMachine, hres, pSvc, pLoc);
	getStorage(localMachine, hres, pSvc, pLoc);
	getCDROM(localMachine, hres, pSvc, pLoc);
	getAudio(localMachine, hres, pSvc, pLoc);
	getUptime(localMachine);
	// Cleanup
	// ========

	pSvc->Release();
	pLoc->Release();
	//pEnumerator->Release();
	CoUninitialize();

	return 0;   // Program successfully completed.
}
void getCPUInfo()
{
	
}
void getCPU(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_Processor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		           // Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		string fullCPUString; //name + @clock
		string maxClock;
		double maxClockInGhZ;
		uint32_t maxClockInMhZ;
		char maxClockBuff[10];
		string processor;

		BstrToStdString(vtProp.bstrVal, processor);
		trimNullTerminator(processor);
		trimWhiteSpace(processor);
		if (processor.find("@",0) == string::npos)
		{
			hr = pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
			maxClockInMhZ = vtProp.uintVal;
			maxClockInGhZ = (double)maxClockInMhZ / 1000;
			sprintf(maxClockBuff, "%.1lf", maxClockInGhZ);
			maxClock = string(maxClockBuff);
			fullCPUString = processor + " @ " + maxClock + " Ghz";
		}
		else
		{
			fullCPUString = processor;
		}
	
		localMachine->setCPU(fullCPUString);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getRAM(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	IEnumWbemClassObject *pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_PhysicalMemory"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		              // Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		//format
		//manufacturer + gb channel ddr @ mhz (x-x-x-x)
		hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
		string manufacturer;
		string clockStr;
		UINT32 clock;
		UINT16 formFactor;
		string name;
		string capacityStr;
		string formFactorStr;
		string memoryTypeStr;
		UINT16 memoryType;
		BstrToStdString(vtProp.bstrVal, manufacturer);
		trimNullTerminator(manufacturer);
		trimWhiteSpace(manufacturer);
		uint64_t capacity;
		double capacityDouble;
		char capacityStrBuff[10];
		char clockStrBuff[10];
		capacityStr = getActualPhysicalMemory(hres, pSvc,pLoc);
		
		hr = pclsObj->Get(L"FormFactor", 0, &vtProp, 0, 0);
		formFactor = vtProp.uintVal;
		formFactorStr = RAMFormFactors[formFactor];
		
		hr = pclsObj->Get(L"MemoryType", 0, &vtProp, 0, 0);
		memoryType = vtProp.uintVal;
		memoryTypeStr = RAMMemoryTypes[memoryType];
		hr = pclsObj->Get(L"Speed", 0, &vtProp, 0, 0);	
		clock = vtProp.uintVal;
		sprintf(clockStrBuff,"%d",clock);
		clockStr = string(clockStrBuff);
		localMachine->setRAM(manufacturer + " " + capacityStr +
		" GiB " + formFactorStr + " "+memoryTypeStr+" "+clockStr+"Mhz");

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getOS(SystemInfo *localMachine, 
			HRESULT hres, IWbemServices *pSvc,
			IWbemLocator *pLoc)
{
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_OperatingSystem"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		             // Program has failed.
	}

	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		string fullOSstring;
		string OSName;
		string OSArchitecture;

		BstrToStdString(vtProp.bstrVal, OSName);
		hr = pclsObj->Get(L"OSArchitecture", 0, &vtProp, 0, 0);
		BstrToStdString(vtProp.bstrVal, OSArchitecture);
		int garbageIndex = OSName.find("|");
		OSName = OSName.erase(garbageIndex, OSName.length() - garbageIndex);
		fullOSstring = OSName;

		fullOSstring.append(" " + OSArchitecture);
		localMachine->setOS(fullOSstring);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getMB(SystemInfo *localMachine,
		HRESULT hres, IWbemServices *pSvc,
		IWbemLocator *pLoc)
{
	IEnumWbemClassObject* pEnumerator = NULL;
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_BaseBoard"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);

		//product - m5a97 r2.0
		string manufacturer;
		string product;
		BstrToStdString(vtProp.bstrVal, manufacturer);
		manufacturer.erase(manufacturer.length() - 1);
		hr = pclsObj->Get(L"Product", 0, &vtProp, 0, 0);
		BstrToStdString(vtProp.bstrVal, product);
		localMachine->setMB(manufacturer + " " + product);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
string getActualPhysicalMemory(HRESULT hres, 
	IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	string ram;
	IEnumWbemClassObject *pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_PhysicalMemory"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;
	double accumulatedRAM = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		hr = pclsObj->Get(L"Capacity", 0, &vtProp, 0, 0);
		double cap;
		double capacity;
		
		string temp;
		char tempChar[100];
		BstrToStdString(vtProp.bstrVal, temp);
		strcpy(tempChar,temp.c_str());
		sscanf(tempChar,"%lf",&cap);
		cap/= (pow(1024, 3));
		accumulatedRAM+=cap;
		VariantClear(&vtProp);

		pclsObj->Release();
	}
	char capacityStrBuff[100];

	sprintf(capacityStrBuff, "%.1lf", accumulatedRAM);
	ram = string(capacityStrBuff);
	pEnumerator->Release();
	return ram;
}
void getGPU(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	IEnumWbemClassObject *pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_VideoController"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		// Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		//name - name
		//memory - AdapterRAM
		//vendor - 
		hr = pclsObj->Get(L"AdapterRAM", 0, &vtProp, 0, 0);
		string finalAdapterString;
		string name;
		UINT32 vramBytes;
		string vrammegabytesStr;
		char vRamCharBuff[50];
		double vRAMmegaBytes;
		
		vramBytes = vtProp.uintVal;
		vRAMmegaBytes = (double) vramBytes/pow(1024,2);
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		BstrToStdString(vtProp.bstrVal, name);
		trimNullTerminator(name);
		sprintf(vRamCharBuff,"%.0lf MB",vRAMmegaBytes);
		vrammegabytesStr = string(vRamCharBuff);
		finalAdapterString = name+" "+vrammegabytesStr;

		localMachine->addGPUDevice(finalAdapterString);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getMonitor(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	IEnumWbemClassObject *pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DesktopMonitor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		// Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		//MonitorManufacturer
		//Name
		//DELL P2014H(DP) (1600x900@60Hz)
		string finalMonitorString;
		string monitorName;
		string resAndFreqStr;
		char resAndFreqBuff[50];
		UINT32 dimensionsAndFrequency[3];
		getDimensionsAndFrequency(hres, pSvc, pLoc,dimensionsAndFrequency);
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		BstrToStdString(vtProp.bstrVal, monitorName);

		trimNullTerminator(monitorName);
	
		sprintf(resAndFreqBuff,"(%dx%d@%dHz)",dimensionsAndFrequency[0], 
										dimensionsAndFrequency[1],
										dimensionsAndFrequency[2]);
		resAndFreqStr = string(resAndFreqBuff);
		finalMonitorString = monitorName + resAndFreqStr;
		localMachine->addDisplayDevice(finalMonitorString);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getDimensionsAndFrequency(HRESULT hres,
	IWbemServices *pSvc,
	IWbemLocator *pLoc, UINT *arr)
{
	IEnumWbemClassObject *pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DisplayConfiguration"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;
	double accumulatedRAM = 0;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		arr[0] = GetSystemMetrics(SM_CXSCREEN);
		arr[1] = GetSystemMetrics(SM_CYSCREEN);
		hr = pclsObj->Get(L"DisplayFrequency", 0, &vtProp, 0, 0);
		arr[2] = vtProp.uintVal;

	
		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getStorage(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	IEnumWbemClassObject* pEnumerator = NULL;
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_DiskDrive"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		// Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		string storageFullString;
		string manufacturerName;
		string modelName;
		string capacityStr;
		UINT64 capacityBytes;
		string capacityGiBStr;
		UINT64 capacityGiBDbl;
		char capacity[256];
		// Get the value of the Name property
		hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);

		//product - m5a97 r2.0
		BstrToStdString(vtProp.bstrVal, modelName);
		manufacturerName = parseDiskStorageName(modelName);
		trimNullTerminator(modelName);

		hr = pclsObj->Get(L"Size", 0, &vtProp, 0, 0);
		
		BstrToStdString(vtProp.bstrVal, capacityStr);
		capacityBytes = stoull(capacityStr);
		capacityGiBDbl = capacityBytes/pow(1024,3);
		capacityGiBStr = convertUIntToString(capacityGiBDbl);

		storageFullString = manufacturerName +" "+ modelName + " ("+capacityGiBStr+" GB)";

		localMachine->addStorageMedium(storageFullString);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getCDROM(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{

	IEnumWbemClassObject* pEnumerator = NULL;
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_CDROMDrive"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		// Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		string cdRomCaption;
		// Get the value of the Name property
		hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);

		BstrToStdString(vtProp.bstrVal, cdRomCaption);
		trimNullTerminator(cdRomCaption);
		localMachine->addCDROMDevice(cdRomCaption);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getAudio(SystemInfo *localMachine,
	HRESULT hres, IWbemServices *pSvc,
	IWbemLocator *pLoc)
{
	IEnumWbemClassObject* pEnumerator = NULL;
	pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_SoundDevice"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		// Program has failed.
	}
	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;
		string soundCaption;
		// Get the value of the Name property
		hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);

		BstrToStdString(vtProp.bstrVal, soundCaption);
		trimNullTerminator(soundCaption);
		localMachine->setAudio(soundCaption);

		VariantClear(&vtProp);

		pclsObj->Release();
	}
	pEnumerator->Release();
}
void getUptime(SystemInfo *localMachine)
{

		VARIANT vtProp;
		string soundCaption;
		string uptimeStr;
		UINT64 uptimeMilliseconds = GetTickCount64();
		UINT64 uptimeHours;
		UINT64 uptimeDays;
		char *format;
		char formattedTimeString[256];
		uptimeMilliseconds/=1000;
		uptimeHours = uptimeMilliseconds/3600;
		if (uptimeHours > 0)
		{
			if (uptimeHours > 24)
			{
				uptimeDays = uptimeHours / 24;
				uptimeHours -= uptimeDays * 24;
				if (!uptimeHours%24)
				{
					format = "%llu days";
				}
				else
				{
					format = "%llu days and %llu hours";
				}
			}
			else
			{
				format = "%u hours";
			}
			sprintf(formattedTimeString, format, uptimeDays, uptimeHours);
			uptimeStr = string(formattedTimeString);
		}
		else
		{
			uptimeStr = "Less than an hour";
		}
		localMachine->setUptime(uptimeStr);
}