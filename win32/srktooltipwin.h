#if !defined(SRKTOOLTIPWIN_H)
#define SRKTOOLTIPWIN_H

extern bool SONORK_TT_Init();

extern HWND SONORK_TT_Handle();
extern bool SONORK_TT_AddCtrl(HWND,UINT count,UINT*id_list);
extern bool SONORK_TT_AddCtrl(HWND,HWND);
extern bool SONORK_TT_AddRect(HWND,UINT id, RECT*rect);
extern void SONORK_TT_Del(HWND);

extern void SONORK_TT_Exit();

#endif
