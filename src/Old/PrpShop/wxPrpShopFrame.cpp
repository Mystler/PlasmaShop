#include "wxPrpShopFrame.h"
#include "PlasmaTreeItem.h"
#include "../../rc/PlasmaShop.xpm"
#include "../../rc/Toolbar.xpm"

#include <Debug/plDebug.h>
#include <PRP/plSceneNode.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/sysopt.h>
#include <wx/config.h>
#include <wx/artprov.h>

wxLocationInfo::wxLocationInfo(const wxTreeItemId& tid, const wxString& filename)
              : fTreeId(tid), fFilename(filename)
{ }

wxLocationInfo::wxLocationInfo(const wxLocationInfo& init)
              : fTreeId(init.fTreeId), fFilename(init.fFilename)
{ }

wxLocationInfo::wxLocationInfo()
{ }


BEGIN_EVENT_TABLE(wxPrpShopFrame, wxFrame)
    EVT_MENU(wxID_EXIT, wxPrpShopFrame::OnExitClick)
    EVT_MENU(wxID_OPEN, wxPrpShopFrame::OnOpenClick)
    EVT_MENU(wxID_SAVE, wxPrpShopFrame::OnSaveClick)
    EVT_MENU(wxID_SAVEAS, wxPrpShopFrame::OnSaveAsClick)
    EVT_MENU(ID_VIEW_POINTS, wxPrpShopFrame::OnViewPointsClick)
    EVT_MENU(ID_VIEW_WIRE, wxPrpShopFrame::OnViewWireClick)
    EVT_MENU(ID_VIEW_FLAT, wxPrpShopFrame::OnViewFlatClick)
    EVT_MENU(ID_VIEW_TEXTURED, wxPrpShopFrame::OnViewTexturedClick)
    EVT_MENU(ID_VIEW_FORCE2SIDED, wxPrpShopFrame::OnViewForce2SidedClick)
    EVT_CLOSE(wxPrpShopFrame::OnClose)

    EVT_TREE_SEL_CHANGED(ID_OBJTREE, wxPrpShopFrame::OnTreeChanged)
END_EVENT_TABLE()

wxPrpShopFrame::wxPrpShopFrame(wxApp* owner)
    : wxFrame(NULL, wxID_ANY, wxT("PrpShop 1.0"), wxDefaultPosition, wxSize(800, 600)),
      fOwner(owner), fCurObject(NULL), fCurPage(NULL)
{
    for (size_t i=0; i<TYPESPACE_MAX; i++)
        fEditorPageMemory[i] = 0;

    fHSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    fVSplitter = new wxSplitterWindow(fHSplitter, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);

    // GUI Elements
    fObjTree = new wxTreeCtrl(fHSplitter, ID_OBJTREE, wxDefaultPosition,
                              wxSize(240, -1),
                              wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT);
    fObjTree->AddRoot(wxT(""));
    fViewerPane = new wxPanel(fVSplitter, wxID_ANY);
    fPropertyBook = new wxNotebook(fVSplitter, ID_PROPERTYBOOK, wxDefaultPosition,
                                   wxSize(-1, 220), wxNB_TOP);

    // Toolbar
    wxSystemOptions::SetOption(wxT("msw.remap"), 0);
    wxToolBar* toolBar = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, wxTB_FLAT);
    toolBar->AddTool(wxID_OPEN, wxT("Open"),
                     wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR),
                     wxNullBitmap, wxITEM_NORMAL, wxT("Open PRP"),
                     wxT("Open a page for editing"));
    toolBar->AddTool(wxID_SAVE, wxT("Save"),
                     wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR),
                     wxNullBitmap, wxITEM_NORMAL, wxT("Save PRP"),
                     wxT("Save changes to active PRP"));
    toolBar->AddSeparator();
    toolBar->AddCheckTool(ID_VIEW_POINTS, wxT("Points"), wxBitmap(XPM_point), wxNullBitmap,
                          wxT("Points"), wxT("Render as points"));
    toolBar->AddCheckTool(ID_VIEW_WIRE, wxT("Wireframe"), wxBitmap(XPM_wire), wxNullBitmap,
                          wxT("Wireframe"), wxT("Render as wireframe"));
    toolBar->AddCheckTool(ID_VIEW_FLAT, wxT("Flat"), wxBitmap(XPM_flat), wxNullBitmap,
                          wxT("Flat"), wxT("Render as flat polygons"));
    toolBar->AddCheckTool(ID_VIEW_TEXTURED, wxT("Textured"), wxBitmap(XPM_tex), wxNullBitmap,
                          wxT("Textured"), wxT("Render as textured polygons"));
    toolBar->Realize();
    SetToolBar(toolBar);

    // Menu, Status Bar
    wxMenu* mnuFile = new wxMenu();
    mnuFile->Append(wxID_OPEN, wxT("&Open...\tCtrl+O"), wxT("Open a page for editing"));
    mnuFile->Append(wxID_SAVE, wxT("&Save\tCtrl+S"), wxT("Save chages to active PRP"));
    mnuFile->Append(wxID_SAVEAS, wxT("Save &As..."), wxT("Save chages to a different PRP file"));
    mnuFile->AppendSeparator();
    mnuFile->Append(wxID_EXIT, wxT("E&xit\tAlt+F4"), wxT("Exit PrpShop"));

    wxMenu* mnuViewRender = new wxMenu();
    mnuViewRender->AppendCheckItem(ID_VIEW_POINTS, wxT("&Points"), wxT("Render as points"));
    mnuViewRender->AppendCheckItem(ID_VIEW_WIRE, wxT("&Wireframe"), wxT("Render as wireframe"));
    mnuViewRender->AppendCheckItem(ID_VIEW_FLAT, wxT("&Flat"), wxT("Render as flat polygons"));
    mnuViewRender->AppendCheckItem(ID_VIEW_TEXTURED, wxT("&Textured"), wxT("Render as textured polygons"));

    wxMenu* mnuView = new wxMenu();
    mnuView->AppendSubMenu(mnuViewRender, wxT("&Render As"));
    mnuView->AppendCheckItem(ID_VIEW_FORCE2SIDED, wxT("Force &2-Sided"), wxT("Always render polygons 2-sided"));

    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(mnuFile, wxT("&File"));
    menuBar->Append(mnuView, wxT("&View"));

    SetMenuBar(menuBar);
    SetStatusBar(new wxStatusBar(this, wxID_ANY));

    // Layout
    fObjTree->SetMinSize(wxSize(0, 0));
    fPropertyBook->SetMinSize(wxSize(0, 0));
    fVSplitter->SetMinSize(wxSize(0, 0));

    fVSplitter->SetSashGravity(1.0);
    fVSplitter->SplitHorizontally(fViewerPane, fPropertyBook, -220);
    fHSplitter->SetSashGravity(0.0);
    fHSplitter->SplitVertically(fObjTree, fVSplitter, 240);

    // Miscellaneous
    SetIcon(wxIcon(XPM_PlasmaShop));
    fObjTree->SetImageList(GetTypeImgList());

    // Set up the Resource Manager
    fResMgr = new plResManager();

    // User configuration stuff
    wxConfigBase* cfg = wxConfigBase::Get();
    cfg->SetPath(wxT("/PrpShop"));

    long width, height, left, top;
    bool maximized;
    cfg->Read(wxT("Width"), &width, -1);
    cfg->Read(wxT("Height"), &height, -1);
    cfg->Read(wxT("Left"), &left, -1);
    cfg->Read(wxT("Top"), &top, -1);
    cfg->Read(wxT("Maximized"), &maximized, false);
    if (height != -1 && width != -1)
        SetSize(width, height);
    else if (wxGetDisplaySize().GetHeight() > 768)
        SetSize(1024, 768);
    if (left != -1 && top != -1)
        SetPosition(wxPoint(left, top));
    Maximize(maximized);

    long vSplitPos, hSplitPos;
    cfg->Read(wxT("VSplit"), &vSplitPos, -1);
    cfg->Read(wxT("HSplit"), &hSplitPos, -1);
    if (vSplitPos != -1)
        fVSplitter->SetSashPosition(vSplitPos);
    if (hSplitPos != -1)
        fHSplitter->SetSashPosition(hSplitPos);

    // Menu options
    cfg->Read(wxT("DrawMode"), &fCachedDrawMode, wxPrpCanvas::DRAW_TEXTURED);
    UpdateMenuAndToolbar();
}

wxPrpShopFrame::~wxPrpShopFrame()
{
    // Save the configuration
    wxConfigBase* cfg = wxConfigBase::Get();
    cfg->SetPath(wxT("/PrpShop"));

    if (!IsMaximized()) {
        cfg->Write(wxT("Width"), GetSize().GetWidth());
        cfg->Write(wxT("Height"), GetSize().GetHeight());
        cfg->Write(wxT("Left"), GetPosition().x);
        cfg->Write(wxT("Top"), GetPosition().y);
    }
    cfg->Write(wxT("Maximized"), IsMaximized());

    cfg->Write(wxT("VSplit"), fVSplitter->GetSashPosition());
    cfg->Write(wxT("HSplit"), fHSplitter->GetSashPosition());

    // Menu settings
    cfg->Write(wxT("DrawMode"), fCachedDrawMode);

    // Now clean up what's left on the form
    if (fCurObject != NULL)
        delete fCurObject;
    delete fResMgr;
}

void wxPrpShopFrame::LoadFile(const wxString& filename)
{
    wxFileName fn(filename);

    if (!fn.FileExists()) {
        wxMessageBox(wxString::Format(wxT("Error: File %s not found!"), filename.wx_str()),
                     wxT("Error"), wxOK | wxICON_ERROR);
        return;
    }

    try {
        wxString ext = fn.GetExt();
        if (ext.CmpNoCase(wxT("age")) == 0) {
            plAgeInfo* age = fResMgr->ReadAge(fn.GetFullPath().mb_str(), true);
            for (size_t i=0; i<age->getNumPages(); i++)
                LoadPage(fResMgr->FindPage(age->getPageLoc(i, fResMgr->getVer())),
                                           wxString(age->getPageFilename(i, fResMgr->getVer()), wxConvUTF8));
            for (size_t i=0; i<age->getNumCommonPages(fResMgr->getVer()); i++)
                LoadPage(fResMgr->FindPage(age->getCommonPageLoc(i, fResMgr->getVer())),
                                           wxString(age->getCommonPageFilename(i, fResMgr->getVer()), wxConvUTF8));
        } else if (ext.CmpNoCase(wxT("prp")) == 0) {
            plPageInfo* page = fResMgr->ReadPage(fn.GetFullPath().mb_str());
            wxTreeItemId pageId = LoadPage(page, fn.GetFullPath()).fTreeId;
            wxTreeItemId ageId = fObjTree->GetItemParent(pageId);
            PlasmaTreeItem* ageInfo = (PlasmaTreeItem*)fObjTree->GetItemData(ageId);
            if (ageInfo != NULL && ageInfo->getAge() != NULL) {
                if (!ageInfo->getAge()->fHasTextures) {
                    plString texPath = (const char*)fn.GetFullPath().mb_str();
                    texPath = texPath.beforeLast(PATHSEP) + PATHSEP + page->getAge();
                    if (fResMgr->getVer() < pvEoa)
                        texPath += "_District";
                    texPath += "_Textures.prp";
                    if (wxFileExists(wxString(texPath, wxConvUTF8)))
                        LoadPage(fResMgr->ReadPage(texPath), wxString(texPath, wxConvUTF8));
                }
                if (!ageInfo->getAge()->fHasBuiltIn) {
                    plString biPath = (const char*)fn.GetFullPath().mb_str();
                    biPath = biPath.beforeLast(PATHSEP) + PATHSEP + page->getAge();
                    if (fResMgr->getVer() < pvEoa)
                        biPath += "_District";
                    biPath += "_BuiltIn.prp";
                    if (wxFileExists(wxString(biPath, wxConvUTF8)))
                        LoadPage(fResMgr->ReadPage(biPath), wxString(biPath, wxConvUTF8));
                }
            }
        } else {
            wxMessageBox(wxString::Format(wxT("%s: Unsupported file type!"), filename.wx_str()),
                         wxT("Error"), wxOK | wxICON_ERROR, this);
        }
    } catch (std::exception& e) {
        wxMessageBox(wxString::Format(wxT("Error loading %s: %s"),
                     filename.wx_str(), wxString(e.what(), wxConvUTF8).wx_str()),
                     wxT("Error"), wxOK | wxICON_ERROR, this);
    }

    fObjTree->SortChildren(fObjTree->GetRootItem());
}

wxLocationInfo wxPrpShopFrame::LoadPage(plPageInfo* page, const wxString& filename)
{
    if (fLoadedLocations[page->getLocation()].fTreeId.IsOk()) {
        fLoadedLocations[page->getLocation()].fFilename = filename;
        return fLoadedLocations[page->getLocation()];
    }

    wxString ageName = wxString(page->getAge().cstr(), wxConvUTF8);
    wxString pageName = wxString(page->getPage().cstr(), wxConvUTF8);
    wxTreeItemIdValue scookie;
    wxTreeItemId ageId = fObjTree->GetFirstChild(fObjTree->GetRootItem(), scookie);
    while (ageId.IsOk()) {
        if (fObjTree->GetItemText(ageId) == ageName)
            break;
        ageId = fObjTree->GetNextChild(fObjTree->GetRootItem(), scookie);
    }
    if (!ageId.IsOk())
        ageId = fObjTree->AppendItem(fObjTree->GetRootItem(), ageName, 1, -1,
                                     new PlasmaTreeItem(TreeAgeInfo(ageName)));

    std::vector<plKey> keys;

    // Special cases: Textures and BuiltIn:
    if (page->getLocation().getPageNum() == -1) {
        wxTreeItemId texFolderId = fObjTree->InsertItem(ageId, 0, wxT("Textures"), ico_folder,
                                                        -1, new PlasmaTreeItem(page));
        wxTreeItemId envmapId = fObjTree->AppendItem(texFolderId, wxT("Environment Maps"), ico_folder);
        wxTreeItemId mipmapId = fObjTree->AppendItem(texFolderId, wxT("Mipmaps"), ico_folder);

        keys = fResMgr->getKeys(page->getLocation(), kCubicEnvironmap);
        for (size_t i=0; i<keys.size(); i++)
            TreeAddObject(fObjTree, envmapId, fResMgr, keys[i]);
        keys = fResMgr->getKeys(page->getLocation(), kMipmap);
        for (size_t i=0; i<keys.size(); i++)
            TreeAddObject(fObjTree, mipmapId, fResMgr, keys[i]);
        fObjTree->SortChildren(envmapId);
        fObjTree->SortChildren(mipmapId);

        ((PlasmaTreeItem*)fObjTree->GetItemData(ageId))->getAge()->fHasTextures = true;
        fLoadedLocations[page->getLocation()] = wxLocationInfo(texFolderId, filename);
    } else if (page->getLocation().getPageNum() == -2) {
        keys = fResMgr->getKeys(page->getLocation(), kSceneObject);
        wxTreeItemId biFolderId;
        if (keys.size() > 1)
            plDebug::Warning("Got multiple Scene Objects in a BuiltIn PRP");
        if (keys.size() >= 1) {
            if (((PlasmaTreeItem*)fObjTree->GetItemData(ageId))->getAge()->fHasTextures)
                biFolderId = fObjTree->InsertItem(ageId, 1, wxT("Built-In"), ico_folder,
                                                  -1, new PlasmaTreeItem(page));
            else
                biFolderId = fObjTree->InsertItem(ageId, 0, wxT("Built-In"), ico_folder,
                                                  -1, new PlasmaTreeItem(page));
            TreeAddObject(fObjTree, biFolderId, fResMgr, keys[0]);
        }

        ((PlasmaTreeItem*)fObjTree->GetItemData(ageId))->getAge()->fHasBuiltIn = true;
        fLoadedLocations[page->getLocation()] = wxLocationInfo(biFolderId, filename);
    } else {
        // Normal page
        wxTreeItemId pageId = fObjTree->AppendItem(ageId, pageName, ico_page, -1,
                                                   new PlasmaTreeItem(page));
        fLoadedLocations[page->getLocation()] = wxLocationInfo(pageId, filename);

        // Local Textures (for PyPRP pages and such)
        keys = fResMgr->getKeys(page->getLocation(), kCubicEnvironmap);
        if (keys.size() > 0) {
            wxTreeItemId texFolderId = fObjTree->InsertItem(pageId, 0, wxT("Environment Maps"), ico_folder);
            for (size_t i=0; i<keys.size(); i++)
                TreeAddObject(fObjTree, texFolderId, fResMgr, keys[i]);
            fObjTree->SortChildren(texFolderId);
        }
        keys = fResMgr->getKeys(page->getLocation(), kMipmap);
        if (keys.size() > 0) {
            wxTreeItemId texFolderId = fObjTree->InsertItem(pageId, 0, wxT("Textures"), ico_folder);
            for (size_t i=0; i<keys.size(); i++)
                TreeAddObject(fObjTree, texFolderId, fResMgr, keys[i]);
            fObjTree->SortChildren(texFolderId);
        }

        // All materials
        keys = fResMgr->getKeys(page->getLocation(), kGMaterial);
        if (keys.size() > 0) {
            wxTreeItemId folder = fObjTree->InsertItem(pageId, 0, wxT("Materials"), ico_folder);
            for (size_t i=0; i<keys.size(); i++)
                TreeAddObject(fObjTree, folder, fResMgr, keys[i]);
            fObjTree->SortChildren(folder);
        }

        // The Scene Node
        TreeAddObject(fObjTree, pageId, fResMgr, fResMgr->getSceneNode(page->getLocation())->getKey());
    }

    return fLoadedLocations[page->getLocation()];
}

void wxPrpShopFrame::OnExitClick(wxCommandEvent& evt)
{
    Close();
}

void wxPrpShopFrame::OnOpenClick(wxCommandEvent& evt)
{
    static const wxString kFilter =
        wxT("All supported files|*.age;*.prp|")
        wxT("Age files (*.age)|*.age|")
        wxT("Page files (*.prp)|*.prp");

    wxFileDialog fd(this, wxT("Open file"), wxEmptyString,
                    wxEmptyString, kFilter,
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (fd.ShowModal() != wxID_CANCEL) {
        wxArrayString paths;
        fd.GetPaths(paths);
        for (size_t i=0; i<paths.GetCount(); i++)
            LoadFile(paths[i]);
    }
}

void wxPrpShopFrame::OnSaveClick(wxCommandEvent& evt)
{
    DoDataSave(false);
    plLocation loc = GetActiveLocation();
    if (!loc.isValid()) {
        wxMessageBox(wxT("No PRP file or object selected!"), wxT("Error"),
                     wxOK | wxICON_ERROR);
        return;
    }
    if (fLoadedLocations[loc].fFilename == wxEmptyString) {
        OnSaveAsClick(evt);
        return;
    }
    fResMgr->WritePage((const char*)fLoadedLocations[loc].fFilename.mb_str(),
                       fResMgr->FindPage(loc));
}

void wxPrpShopFrame::OnSaveAsClick(wxCommandEvent& evt)
{
    static const wxString kFilter =
        wxT("Uru Prime / UU Page|*.prp|")
        wxT("Path of the Shell / CC Page|*.prp|")
        wxT("Myst Online Uru Live Page|*.prp|")
        wxT("Myst 5 / Crowthistle Page|*.prp|")
        wxT("Hex Isle Page|*.prp");

    DoDataSave(false);
    plLocation loc = GetActiveLocation();
    if (!loc.isValid()) {
        wxMessageBox(wxT("No PRP file or object selected!"), wxT("Error"),
                     wxOK | wxICON_ERROR);
        return;
    }
    wxFileName name(fLoadedLocations[loc].fFilename);
    wxFileDialog fd(this, wxT("Save PRP"), name.GetPath(),
                    name.GetName(), kFilter,
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    switch (fResMgr->getVer()) {
    case pvPrime:
        fd.SetFilterIndex(0);
        break;
    case pvPots:
        fd.SetFilterIndex(1);
        break;
    case pvLive:
        fd.SetFilterIndex(2);
        break;
    case pvEoa:
        fd.SetFilterIndex(3);
        break;
    case pvHex:
        fd.SetFilterIndex(4);
        break;
    default:
        fd.SetFilterIndex(1);
        break;
    }

    if (fd.ShowModal() == wxID_CANCEL)
        return;

    switch (fd.GetFilterIndex()) {
    case 0:
        fResMgr->setVer(pvPrime, true);
        break;
    case 1:
        fResMgr->setVer(pvPots, true);
        break;
    case 2:
        if (wxMessageBox(wxT("Saving in this format WILL result in loss of PhysX data!"),
                         wxT("Warning"), wxOK | wxCANCEL | wxICON_EXCLAMATION) != wxOK)
            return;
        fResMgr->setVer(pvLive, true);
        break;
    case 3:
        if (wxMessageBox(wxT("Saving in this format may result in loss or corruption of data!"),
                         wxT("Warning"), wxOK | wxCANCEL | wxICON_EXCLAMATION) != wxOK)
            return;
        fResMgr->setVer(pvEoa, true);
        break;
    case 4:
        if (wxMessageBox(wxT("Saving in this format may result in loss or corruption of data!"),
                         wxT("Warning"), wxOK | wxCANCEL | wxICON_EXCLAMATION) != wxOK)
            return;
        fResMgr->setVer(pvHex, true);
        break;
    }

    fResMgr->WritePage((const char*)fd.GetPath().mb_str(), fResMgr->FindPage(loc));
    fLoadedLocations[loc].fFilename = fd.GetPath();
}

void wxPrpShopFrame::OnViewPointsClick(wxCommandEvent& evt)
{
    fCachedDrawMode = (fCachedDrawMode & ~wxPrpCanvas::DRAW_MODEMASK)
                    | wxPrpCanvas::DRAW_POINTS;
    wxConfigBase::Get()->Write(wxT("DrawMode"), fCachedDrawMode);
    UpdateMenuAndToolbar();
    if (fCurObject != NULL)
        fCurObject->Refresh();
}

void wxPrpShopFrame::OnViewWireClick(wxCommandEvent& evt)
{
    fCachedDrawMode = (fCachedDrawMode & ~wxPrpCanvas::DRAW_MODEMASK)
                    | wxPrpCanvas::DRAW_WIRE;
    wxConfigBase::Get()->Write(wxT("DrawMode"), fCachedDrawMode);
    UpdateMenuAndToolbar();
    if (fCurObject != NULL)
        fCurObject->Refresh();
}

void wxPrpShopFrame::OnViewFlatClick(wxCommandEvent& evt)
{
    fCachedDrawMode = (fCachedDrawMode & ~wxPrpCanvas::DRAW_MODEMASK)
                    | wxPrpCanvas::DRAW_FLAT;
    wxConfigBase::Get()->Write(wxT("DrawMode"), fCachedDrawMode);
    UpdateMenuAndToolbar();
    if (fCurObject != NULL)
        fCurObject->Refresh();
}

void wxPrpShopFrame::OnViewTexturedClick(wxCommandEvent& evt)
{
    fCachedDrawMode = (fCachedDrawMode & ~wxPrpCanvas::DRAW_MODEMASK)
                    | wxPrpCanvas::DRAW_TEXTURED;
    wxConfigBase::Get()->Write(wxT("DrawMode"), fCachedDrawMode);
    UpdateMenuAndToolbar();
    if (fCurObject != NULL)
        fCurObject->Refresh();
}

void wxPrpShopFrame::OnViewForce2SidedClick(wxCommandEvent& evt)
{
    fCachedDrawMode = (fCachedDrawMode & wxPrpCanvas::DRAW_MODEMASK)
                    | (evt.IsChecked() ? wxPrpCanvas::DRAW_FORCE2SIDED : 0);
    wxConfigBase::Get()->Write(wxT("DrawMode"), fCachedDrawMode);
    UpdateMenuAndToolbar();
    if (fCurObject != NULL)
        fCurObject->Refresh();
}

void wxPrpShopFrame::OnClose(wxCloseEvent& evt)
{
    if (evt.CanVeto()) {
        int confirm = wxMessageBox(wxT("Any unsaved changes will be lost.  Are you sure you want to quit?"),
                                   wxT("Close PrpShop"), wxYES | wxNO | wxICON_QUESTION);
        if (confirm == wxYES)
            Destroy();
    } else {
        Destroy();
    }
}

void wxPrpShopFrame::OnTreeChanged(wxTreeEvent& evt)
{
    wxTreeItemId itm = evt.GetItem();
    PlasmaTreeItem* data = (PlasmaTreeItem*)fObjTree->GetItemData(itm);
    if (data == NULL)
        return;

    int propPageIdx = fPropertyBook->GetSelection();
    if (fCurObject != NULL && propPageIdx >= 0)
        fEditorPageMemory[fCurObject->getKey()->getType()] = propPageIdx;
    else
        propPageIdx = 0;

    DoDataSave(true);
    fPropertyBook->DeleteAllPages();
    wxWindow* noPreview = new wxStaticText(fVSplitter, wxID_ANY, wxT("No preview available"));
    fVSplitter->ReplaceWindow(fViewerPane, noPreview);
    delete fViewerPane;
    fViewerPane = noPreview;

    if (data->getObject().Exists() && data->getObject().isLoaded()) {
        fCurObject = MakeEditor(fResMgr, data->getObject(), fObjTree, itm);
        fCurObject->AddPropPages(fPropertyBook);
        wxWindow* newPane = fCurObject->MakePreviewPane(fVSplitter);
        if (newPane != NULL) {
            fVSplitter->ReplaceWindow(fViewerPane, newPane);
            delete fViewerPane;
            fViewerPane = newPane;
        }
        propPageIdx = fEditorPageMemory[fCurObject->getKey()->getType()];
    } else if (data->getPage() != NULL) {
        fCurPage = data->getPage();
        wxPanel* nbpage = new wxPanel(fPropertyBook);
        wxString ageName(fCurPage->getAge().cstr(), wxConvUTF8);
        wxString pageName(fCurPage->getPage().cstr(), wxConvUTF8);
        wxString seqPrefix(wxString::Format(wxT("%d"), fCurPage->getLocation().getSeqPrefix()));
        wxString seqSuffix(wxString::Format(wxT("%d"), fCurPage->getLocation().getPageNum()));

        wxStaticText* lbl1 = new wxStaticText(nbpage, wxID_ANY, wxT("Age:"));
        wxStaticText* lbl2 = new wxStaticText(nbpage, wxID_ANY, wxT("Page:"));
        wxStaticText* lbl3 = new wxStaticText(nbpage, wxID_ANY, wxT("Sequence Prefix:"));
        wxStaticText* lbl4 = new wxStaticText(nbpage, wxID_ANY, wxT("Page Number:"));
        wxStaticText* lbl5 = new wxStaticText(nbpage, wxID_ANY, wxT("Flags:"));
        txtAge = new wxTextCtrl(nbpage, wxID_ANY, ageName, wxDefaultPosition, wxSize(200, -1));
        txtPage = new wxTextCtrl(nbpage, wxID_ANY, pageName, wxDefaultPosition, wxSize(200, -1));
        txtSeqPre = new wxTextCtrl(nbpage, wxID_ANY, seqPrefix, wxDefaultPosition, wxSize(40, -1));
        txtSeqSuf = new wxTextCtrl(nbpage, wxID_ANY, seqSuffix, wxDefaultPosition, wxSize(40, -1));

        cbLocalOnly = new wxCheckBox(nbpage, wxID_ANY, wxT("Local Only"));
        cbLocalOnly->SetValue((fCurPage->getLocation().getFlags() & plLocation::kLocalOnly) != 0);
        cbVolatile = new wxCheckBox(nbpage, wxID_ANY, wxT("Volatile"));
        cbVolatile->SetValue((fCurPage->getLocation().getFlags() & plLocation::kVolatile) != 0);
        cbReserved = new wxCheckBox(nbpage, wxID_ANY, wxT("Reserved"));
        cbReserved->SetValue((fCurPage->getLocation().getFlags() & plLocation::kReserved) != 0);
        cbBuiltIn = new wxCheckBox(nbpage, wxID_ANY, wxT("Built-In"));
        cbBuiltIn->SetValue((fCurPage->getLocation().getFlags() & plLocation::kBuiltIn) != 0);
        cbItinerant = new wxCheckBox(nbpage, wxID_ANY, wxT("Itinerant"));
        cbItinerant->SetValue((fCurPage->getLocation().getFlags() & plLocation::kItinerant) != 0);

        wxGridSizer* szrFlags = new wxGridSizer(3, 2, 4, 16);
        szrFlags->Add(cbLocalOnly);
        szrFlags->Add(cbVolatile);
        szrFlags->Add(cbReserved);
        szrFlags->Add(cbBuiltIn);
        szrFlags->Add(cbItinerant);
        szrFlags->Add(0, 0);

        wxFlexGridSizer* props = new wxFlexGridSizer(4, 5, 4, 4);
        props->Add(lbl1);
        props->Add(txtAge);
        props->Add(8, 0);
        props->Add(lbl3);
        props->Add(txtSeqPre);
        props->Add(lbl2);
        props->Add(txtPage);
        props->Add(8, 0);
        props->Add(lbl4);
        props->Add(txtSeqSuf);
        props->Add(0, 10);
        props->Add(0, 10);
        props->Add(0, 10);
        props->Add(0, 10);
        props->Add(0, 10);
        props->Add(lbl5);
        props->Add(szrFlags);
        props->Add(0, 0);
        props->Add(0, 0);
        props->Add(0, 0);

        wxBoxSizer* border = new wxBoxSizer(0);
        border->Add(props, 0, wxALL, 8);
        nbpage->SetSizerAndFit(border);
        fPropertyBook->AddPage(nbpage, wxT("PRP Settings"));
    }

    if (propPageIdx < (int)fPropertyBook->GetPageCount())
        fPropertyBook->ChangeSelection(propPageIdx);
    fPropertyBook->Refresh();

    fObjTree->SetFocus();
}

plLocation wxPrpShopFrame::GetActiveLocation()
{
    if (fCurPage != NULL)
        return fCurPage->getLocation();
    else if (fCurObject != NULL)
        return fCurObject->getKey()->getLocation();
    else
        return plLocation();
}

void wxPrpShopFrame::UpdateMenuAndToolbar()
{
    wxMenuBar* menuBar = GetMenuBar();
    wxToolBar* toolBar = GetToolBar();

    menuBar->Check(ID_VIEW_POINTS, false);
    menuBar->Check(ID_VIEW_WIRE, false);
    menuBar->Check(ID_VIEW_FLAT, false);
    menuBar->Check(ID_VIEW_TEXTURED, false);
    toolBar->ToggleTool(ID_VIEW_POINTS, false);
    toolBar->ToggleTool(ID_VIEW_WIRE, false);
    toolBar->ToggleTool(ID_VIEW_FLAT, false);
    toolBar->ToggleTool(ID_VIEW_TEXTURED, false);

    menuBar->Check(ID_VIEW_FORCE2SIDED, (fCachedDrawMode & wxPrpCanvas::DRAW_FORCE2SIDED) != 0);
    switch (fCachedDrawMode & wxPrpCanvas::DRAW_MODEMASK) {
    case wxPrpCanvas::DRAW_POINTS:
        menuBar->Check(ID_VIEW_POINTS, true);
        toolBar->ToggleTool(ID_VIEW_POINTS, true);
        break;
    case wxPrpCanvas::DRAW_WIRE:
        menuBar->Check(ID_VIEW_WIRE, true);
        toolBar->ToggleTool(ID_VIEW_WIRE, true);
        break;
    case wxPrpCanvas::DRAW_FLAT:
        menuBar->Check(ID_VIEW_FLAT, true);
        toolBar->ToggleTool(ID_VIEW_FLAT, true);
        break;
    case wxPrpCanvas::DRAW_TEXTURED:
        menuBar->Check(ID_VIEW_TEXTURED, true);
        toolBar->ToggleTool(ID_VIEW_TEXTURED, true);
        break;
    }
}

void wxPrpShopFrame::DoDataSave(bool doDelete)
{
    if (fCurObject != NULL) {
        fCurObject->SaveDamage();
        if (doDelete) {
            delete fCurObject;
            fCurObject = NULL;
        }
    }
    if (fCurPage != NULL) {
        fCurPage->setAge((const char*)txtAge->GetValue().mb_str());
        fCurPage->setPage((const char*)txtPage->GetValue().mb_str());

        plLocation newLoc;
        long out;
        txtSeqPre->GetValue().ToLong(&out);
        newLoc.setSeqPrefix(out);
        txtSeqSuf->GetValue().ToLong(&out);
        newLoc.setPageNum(out);
        unsigned short pgFlags = (cbLocalOnly->GetValue() ? plLocation::kLocalOnly : 0)
                               | (cbVolatile->GetValue() ? plLocation::kVolatile : 0)
                               | (cbReserved->GetValue() ? plLocation::kReserved : 0)
                               | (cbBuiltIn->GetValue() ? plLocation::kBuiltIn : 0)
                               | (cbItinerant->GetValue() ? plLocation::kItinerant : 0);
        newLoc.setFlags(pgFlags);
        if (newLoc != fCurPage->getLocation()) {
            fLoadedLocations[newLoc] = fLoadedLocations[fCurPage->getLocation()];
            fLoadedLocations.erase(fLoadedLocations.find(fCurPage->getLocation()));
            fResMgr->ChangeLocation(fCurPage->getLocation(), newLoc);
        }

        if (doDelete)
            fCurPage = NULL;
    }
}