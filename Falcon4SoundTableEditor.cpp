#include "Falcon4SoundTableEditor.h"
#include "Main.h"
#include <wx/wx.h>

//Implementation. entry point of the application.

wxIMPLEMENT_APP(FalconApp);

bool FalconApp::OnInit() {
	FalconFrame* mainframe = new FalconFrame("Hellow World! Teste para o Matheus!");
	mainframe->SetClientSize(800, 700);
	mainframe->Center();
	mainframe->Show();
	return true;
}
