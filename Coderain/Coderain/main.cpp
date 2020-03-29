#include <windows.h>  

#define ID_TIMER 1  
#define STRMAXLEN 25 //一个显示列的最大长度  
#define STRMINLEN 8 //一个显示列的最小长度  
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////////////////////////////////  

typedef struct tagCharChain //整个当作屏幕的一个显示列,这是个双向列表  
{
	struct tagCharChain *prev; //链表的前个元素  
	TCHAR ch; //一个显示列中的一个字符  
	struct tagCharChain *next; //链表的后个元素  
}CharChain, *pCharChain;

typedef struct tagCharColumn
{
	CharChain *head, *current, *point;
	int x, y, iStrLen; //显示列的开始显示的x,y坐标,iStrLen是这个列的长度  
	int iStopTimes, iMustStopTimes; //已经停滞的次数和必须停滞的次数,必须停滞的次数是随机的,其实就是每列的速度，速度=计时器大小*iMustStopTimes。  
}CharColumn, *pCharColumn;

int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("matrix");
	HWND hwnd;//窗口句柄  
	MSG msg;//  
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("此程序必须运行在NT下!"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd = CreateWindow(szAppName, NULL, WS_DLGFRAME | WS_THICKFRAME | WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, SW_SHOWMAXIMIZED); //最大化显示  
	UpdateWindow(hwnd);
	ShowCursor(FALSE); //隐藏鼠标光标  
	srand((int)GetCurrentTime()); //初始化随机数发生器  

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ShowCursor(TRUE); //显示鼠标光标  
	return msg.wParam;
}

TCHAR randomChar() //随机字符产生函数  
{
	return (TCHAR)(rand() % (126 - 33) + 33); //33到126之间  
}

int init(CharColumn *cc, int cyScreen, int x) //初始化,cyScreen屏幕高度  
{
	int j;
	cc->iStrLen = rand() % (STRMAXLEN - STRMINLEN) + STRMINLEN; //显示列的长度  
	cc->x = x + 3; //显示列的开始显示的x坐标  
	cc->y = rand() % 3 ? rand() % cyScreen : 0; //显示列的开始显示的y坐标  
	cc->iMustStopTimes = rand() % 6;//cc->iMustStopTimes = rand()%(200-100)+100;读者可以把速度调慢点，我调的是1s~2s，注意此时计时器需要调为10ms  
	cc->iStopTimes = 0;
	cc->head = cc->current = (pCharChain)calloc(cc->iStrLen, sizeof(CharChain)); //分配连续iStrLen个CharChain类型空间  

	for (j = 0; j < cc->iStrLen - 1; j++)//遍历 iStrLen-1 次  
	{
		cc->current->prev = cc->point; //cc->point一个显示列的前个元素  
		cc->current->ch = '\0';
		cc->current->next = cc->current + 1; //cc->current+1一个显示列的后个元素  
		cc->point = cc->current++; //cc->point = cc->current; cc->current++;  
	}
	cc->current->prev = cc->point; //最后一个节点  
	cc->current->ch = '\0';
	cc->current->next = cc->head;
	cc->head->prev = cc->current; //头节点的前一个为此链的最后一个元素  

	cc->current = cc->point = cc->head; //free掉申请的内存要用current当参数  
	cc->head->ch = randomChar(); //对链表头的元素填充  

	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	//j 为一个显示链中除链表头外的在屏幕上显示的y坐标,temp绿色过度到黑色之用  
	//ctn 用来确定一个显示链是否向下前进,如果等待次数超过必须等待的次数,ctn就代表要向下前进  
	int i, j, temp, ctn;
	static HDC hdcMem;
	HFONT hFont;
	static HBITMAP hBitmap;
	static int cxScreen, cyScreen; //屏幕的宽度 高度.  
	static int iFontWidth = 10, iFontHeight = 15, iColumnCount; //字体的宽度 高度, 列数  
	static CharColumn *ccChain;

	switch (message)
	{
		//系统给消息处理程序发送的消息，每次窗口创建，有且只有1个WM_CREATE消息，所以只运行1次，可以在这里初始化滚动条，获取字体宽度、高度，创建子窗口、控件等等  
	case WM_CREATE:
		cxScreen = GetSystemMetrics(SM_CXSCREEN); //屏幕宽度  
		cyScreen = GetSystemMetrics(SM_CYSCREEN); //屏幕高度  

												  //hwnd调用，计时器命名为ID_TIMER，时间间隔10ms。给读者一个小技巧，因为10ms太小，读者难免很难发现玄机，所以建议读者在理解代码时将时间改为1000ms  
		SetTimer(hwnd, ID_TIMER, 10, NULL);
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		hBitmap = CreateCompatibleBitmap(hdc, cxScreen, cyScreen);
		SelectObject(hdcMem, hBitmap);
		ReleaseDC(hwnd, hdc);

		//创建字体  
		hFont = CreateFont(iFontHeight, iFontWidth - 5, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DRAFT_QUALITY, FIXED_PITCH | FF_SWISS, TEXT("Fixedsys"));

		SelectObject(hdcMem, hFont);
		DeleteObject(hFont);

		SetBkMode(hdcMem, TRANSPARENT); //设置背景模式为透明  
		iColumnCount = cxScreen / (iFontWidth * 3 / 2); //屏幕所显示字母雨的列数  
		ccChain = (pCharColumn)calloc(iColumnCount, sizeof(CharColumn));

		for (i = 0; i < iColumnCount; i++)
			init(ccChain + i, cyScreen, (iFontWidth * 3 / 2)*i);
		return 0;

		//计时器间隔10ms，该消息每隔10ms触发一次  
	case WM_TIMER:
		hdc = GetDC(hwnd);
		PatBlt(hdcMem, 0, 0, cxScreen, cyScreen, BLACKNESS); //将内存设备映像刷成黑色,不理解含义可以注释此句再运行，对比一下即可  

		for (i = 0; i < iColumnCount; i++)
		{
			ctn = (ccChain + i)->iStopTimes++ > (ccChain + i)->iMustStopTimes;

			(ccChain + i)->point = (ccChain + i)->head; //point用于遍历整个显示列  

			SetTextColor(hdcMem, RGB(255, 255, 255));//第一个字符显示为白色  
			TextOut(hdcMem, (ccChain + i)->x, (ccChain + i)->y, &((ccChain + i)->point->ch), 1);

			j = (ccChain + i)->y;
			(ccChain + i)->point = (ccChain + i)->point->next;
			//遍历整个显示列,将这个显示列里的字符从下往上显示  
			temp = 0; //temp绿色过度到黑色之用  

					  //注意第二个判断条件，init函数里除了head节点，其他节点都是'\0'，正是第二个判断才给我们运行起初字母是一个一个增多的效果，读者可以换成下面的句子运行看看  
					  //while ((ccChain + i)->point != (ccChain + i)->head)  
			while ((ccChain + i)->point != (ccChain + i)->head && (ccChain + i)->point->ch)
			{
				SetTextColor(hdcMem, RGB(0, 255 - (255 * (temp++) / (ccChain + i)->iStrLen), 0));
				TextOut(hdcMem, (ccChain + i)->x, j -= iFontHeight, &((ccChain + i)->point->ch), 1);
				(ccChain + i)->point = (ccChain + i)->point->next;
			}

			if (ctn)
				(ccChain + i)->iStopTimes = 0;
			else
				continue;

			(ccChain + i)->y += iFontHeight; //下次开始显示的y坐标 为当前的y坐标加上一个字符的高度  

											 //如果开始显示的y坐标减去 整个显示列的长度超过了屏幕的高度  
			if ((ccChain + i)->y - (ccChain + i)->iStrLen*iFontHeight > cyScreen)
			{
				free((ccChain + i)->current);
				init(ccChain + i, cyScreen, (iFontWidth * 3 / 2)*i);
			}

			//链表的头 为此链表的前个元素,因为下次开始显示的时候 就相当于在整个显示列的开头添加个元素,然后再开始往上显示  
			(ccChain + i)->head = (ccChain + i)->head->prev;
			(ccChain + i)->head->ch = randomChar();
		}

		BitBlt(hdc, 0, 0, cxScreen, cyScreen, hdcMem, 0, 0, SRCCOPY);
		ReleaseDC(hwnd, hdc);
		return 0;

	case WM_RBUTTONDOWN:
		KillTimer(hwnd, ID_TIMER);
		return 0;

	case WM_RBUTTONUP:
		SetTimer(hwnd, ID_TIMER, 10, NULL);
		return 0;

		//处理善后工作  
	case WM_KEYDOWN: //任意键退出  
	case WM_LBUTTONDOWN: //点击鼠标退出  
	case WM_DESTROY:
		KillTimer(hwnd, ID_TIMER);
		DeleteObject(hBitmap);
		DeleteDC(hdcMem);

		for (i = 0; i < iColumnCount; i++)
			free((ccChain + i)->current);
		free(ccChain);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}