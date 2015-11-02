#ifndef _NOTIFY_H_
#define _NOTIFY_H_

#ifdef __cplusplus
extern "C" {
#endif
typedef enum _HARDERROR_RESPONSE_OPTION { 
		OptionAbortRetryIgnore, 
		OptionOk, 
		OptionOkCancel, 
		OptionRetryCancel, 
		OptionYesNo, 
		OptionYesNoCancel, 
		OptionShutdownSystem, 
		OptionExplorerTrayBaloon, 
		OptionCancelTryAgainContinue 
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION; 

typedef enum _HARDERROR_RESPONSE { 
		ResponseReturnToCaller, 
		ResponseNotHandled, 
		ResponseAbort, 
		ResponseCancel, 
		ResponseIgnore, 
		ResponseNo, 
		ResponseOk, 
		ResponseRetry, 
		ResponseYes, 
		ResponseTryAgain, 
		ResponseContinue 
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;


#define MB_ICONHAND                 0x00000010L
#define MB_ICONQUESTION             0x00000020L
#define MB_ICONEXCLAMATION          0x00000030L
#define MB_ICONASTERISK             0x00000040L

#if(WINVER >= 0x0400)
#define MB_USERICON                 0x00000080L
#define MB_ICONWARNING              MB_ICONEXCLAMATION
#define MB_ICONERROR                MB_ICONHAND
#endif /* WINVER >= 0x0400 */

#define MB_ICONINFORMATION          MB_ICONASTERISK
#define MB_ICONSTOP                 MB_ICONHAND
#define MB_SETFOREGROUND            0x00010000L

#define MB_DEFBUTTON1               0x00000000L
#define MB_DEFBUTTON2               0x00000100L
#define MB_DEFBUTTON3               0x00000200L

ULONG 
kMessageBox ( 
	PUNICODE_STRING Message,
	PUNICODE_STRING Caption,
	ULONG ResponseOption,
	ULONG Type
	);

#ifdef __cplusplus
}
#endif

#endif
