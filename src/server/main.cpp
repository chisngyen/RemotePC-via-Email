#include <wx/wx.h>
#include "GUI.h"

class ServerApp : public wxApp {
public:
    virtual bool OnInit() {
        if (!wxApp::OnInit())
            return false;

        ServerFrame* frame = new ServerFrame("Remote Control Server");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ServerApp);