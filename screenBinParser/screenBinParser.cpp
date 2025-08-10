#include <iostream>
#include <vector>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <string>

std::atomic<bool> canRead = false;
std::atomic<bool> canWrite = true;

struct screenData
{
	int w, h;
	std::vector<int> dataSet;
};

screenData threadCopySD;

//prints screendata str5uct to console
void printScreenData(screenData sd)
{
	std::string consoleTxt = "";
	int l = sd.dataSet.size();
	int dsc = 0;
	for (int i = 0; i < l; i++)
	{
		switch (sd.dataSet[i])
		{
		case 0: consoleTxt += ' '; break;
		case 1: consoleTxt += ':'; break;
		case 2: consoleTxt += '*'; break;
		}
		
		dsc++;
		if (dsc >= sd.w)
		{
			consoleTxt += "\n";
			dsc = 0;
		}
	}
	std::cout << consoleTxt << "\n";
}


//reads screen to a set of integers
screenData runScreenRead(HDC * DC, HDC * DC_MEM, int scrW, int scrH, int pixSkipX, int pixSkipY, int SDW, int SDH)
{
		
	screenData SD = { SDW,SDH,{} };	
	SD.dataSet.resize(SDW*SDH);

	unsigned char* lpBitmapBits;
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = scrW;
	bi.bmiHeader.biHeight = -scrH;  //negative so (0,0) is at top left
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;

	HBITMAP bitmap = ::CreateDIBSection(*DC_MEM, &bi, DIB_RGB_COLORS, (VOID**)&lpBitmapBits, NULL, 0);
	HGDIOBJ oldbmp = ::SelectObject(*DC_MEM, bitmap);

	BitBlt(*DC_MEM, 0, 0, scrW - 1, scrH - 1, *DC, 0, 0, SRCCOPY);
	HBITMAP DC_BITMAP = (HBITMAP)GetCurrentObject(*DC_MEM, OBJ_BITMAP);

	int pitch = sizeof(int) * scrW;

	int opID = 0;

	for (int j = 0; j < scrH - 1; j += pixSkipY)
	{
		for (int i = 0; i < scrW - 1; i += pixSkipX)
		{
			int index = j * pitch;
			index += i * 4;
			
			int b = lpBitmapBits[index + 0];
			int g = lpBitmapBits[index + 1];
			int r = lpBitmapBits[index + 2];

			float pixelAvg = 0.33*((float)r + (float)g + (float)b);

			int v = 0;
			if (pixelAvg > 170) v = 2;
			else if (pixelAvg > 85) v = 1;
			SD.dataSet[opID] = v;

			opID++;
		}
	}

	DeleteObject(DC_BITMAP);

	return SD;
}

//render thread function for console
void renThreadFunc()
{
	while (true)
	{
		if (canRead)
		{
			system("cls");
			printScreenData(threadCopySD);
			canRead = false;
			canWrite = true;
		}
	}
}

//entry point function
int main()
{
	//main data
	int sw = 0;
	int sh = 0;
	int pixelskipBase = 20;

	HDC DC;
	HDC DC_MEM;

	//prep to do screen stuff
    std::cout << "Starting...\n";
	DEVMODE dm;
	dm.dmSize = 1024;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	DC = GetDC(0);
	DC_MEM = CreateCompatibleDC(DC);

	sw = dm.dmPelsWidth;
	sh = dm.dmPelsHeight;

	float sProp = (float)sw / (float)sh;

	int pixelskipX = (int)((float)pixelskipBase / sProp);
	int pixelskipY = pixelskipBase;

	//calc expected screendata volume
	int scrDW = round((float)sw/(float)pixelskipX);
	int scrDH = round((float)sh/(float)pixelskipY);

	//run render threads
	std::thread renThread(renThreadFunc);

	//run main cycle
	while (true)
	{
		screenData scrn = runScreenRead(&DC, &DC_MEM, sw, sh, pixelskipX, pixelskipY, scrDW, scrDH);		
		if (canWrite)
		{
			threadCopySD = scrn;
			canRead = true;
			canWrite = false;
		}
	}

	//end
	renThread.join();
	system("pause");
}