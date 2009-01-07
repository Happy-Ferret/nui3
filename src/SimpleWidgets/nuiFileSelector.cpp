/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "nuiButton.h"
#include "nuiFileSelector.h"
#include "nuiMainWindow.h"
#include "nuiSplitter.h"
#include "nuiWindowManager.h"
#include "nuiPane.h"

//
class nuiFileSelectorNode : public nuiTreeNode
{
public:
  nuiFileSelectorNode(nuiFileSelector* pSelector, const nglPath& rPath, nuiWidget* pWidget);
  virtual ~nuiFileSelectorNode();
  
  virtual void Open(bool Opened);
  virtual bool IsEmpty() const;
  
private:
    nuiFileSelector* mpSelector;
};


nuiFileSelectorNode::nuiFileSelectorNode(nuiFileSelector* pSelector, const nglPath& rPath, nuiWidget* pWidget)
: nuiTreeNode(pWidget)
{
  mpSelector = pSelector;
  SetProperty(_T("Path"), rPath.GetPathName());
}

nuiFileSelectorNode::~nuiFileSelectorNode()
{
  
}




void nuiFileSelectorNode::Open(bool Opened)
{
  if (Opened && !mOpened)
  {
    nuiTreeNode::Open(Opened);
    Clear();
    
    nglPath p(GetProperty(_T("Path")));
    
    std::list<nglPath> children;
    p.GetChildren(&children);
    
    std::list<nglPath>::iterator it = children.begin();
    std::list<nglPath>::iterator end = children.end();
    
    while (it != end)
    {
      nglPath pathName(*it);

      if (pathName.IsVisible())
      {
        nuiTreeNode* pNode = mpSelector->GetNewNode(pathName);
        if (pNode)
          AddChild(pNode);
      }

      ++it;
    }
  }
  else
  {
    nuiTreeNode::Open(Opened);
  }
}

bool nuiFileSelectorNode::IsEmpty() const
{
  nglPath p(GetProperty(_T("Path")));
  return p.IsLeaf();
}






//**************************************************************************************************************









nuiFileSelector::nuiFileSelector(const nglPath& rPath, const nglPath& rRootPath, const nglString& rFilter, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened)
: nuiComposite(), mpFolderList(NULL), mEventSink(this)
{
  std::list<nglString> filters;
  if (rFilter != nglString::Null)
    filters.push_back(rFilter);
  Init(rPath, rRootPath, filters, pEntry, showHiddenFiles, mode, ShowVolumes, Opened);
}

nuiFileSelector::nuiFileSelector(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened)
: nuiComposite(), mpFolderList(NULL), mEventSink(this)
{
  Init(rPath, rRootPath, rFilters, pEntry, showHiddenFiles, mode, ShowVolumes, Opened);  
}

nuiFileSelector::nuiFileSelector(const nglPath& rPath, const nglString& rFilter, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened)
: nuiComposite(), mpFolderList(NULL), mEventSink(this)
{
  std::list<nglString> filters;
  if (rFilter != nglString::Null)
    filters.push_back(rFilter);
  Init(rPath, rPath, filters, pEntry, showHiddenFiles, mode, ShowVolumes, Opened);
}

nuiFileSelector::nuiFileSelector(const nglPath& rPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened)
: nuiComposite(), mpFolderList(NULL), mEventSink(this)
{
  Init(rPath, rPath, rFilters, pEntry, showHiddenFiles, mode, ShowVolumes, Opened);  
}

void nuiFileSelector::Init(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened)
{
  if (SetObjectClass(_T("nuiFileSelector")))  
    InitAttributes();
  
	mpEntry = pEntry;
  mShowHiddenFiles = showHiddenFiles;
  mFilters = rFilters;
  mpInfoBox = NULL;
  
  InitSelector (rPath, rRootPath, mpEntry, mode, ShowVolumes, Opened);
  
  NUI_ADD_EVENT(OK);
  NUI_ADD_EVENT(Cancel);  
}


nuiFileSelector::~nuiFileSelector()
{
}


void nuiFileSelector::InitAttributes()
{
  AddAttribute(new nuiAttribute<nglString>
  (nglString(_T("Filter")), nuiUnitName,
    nuiAttribute<nglString>::GetterDelegate(this, &nuiFileSelector::GetLastFilter),
    nuiAttribute<nglString>::SetterDelegate(this, &nuiFileSelector::AddFilter)));
}


nglString nuiFileSelector::GetLastFilter()
{
  if (mFilters.size() == 0)
    return nglString::Null;
  return mFilters.back();
}

void nuiFileSelector::AddFilter(nglString filter)
{
  mFilters.push_back(filter);
}

void nuiFileSelector::SetFilters(const std::list<nglString>& rFilters)
{
  mFilters = rFilters;
}

const std::list<nglString>& nuiFileSelector::GetFilters()
{
  return mFilters;
}



//******************************************************************


void nuiFileSelector::InitSelector (const nglPath& rPath, const nglPath& rRootPath, nuiEditLine* pEntry, DisplayMode mode, bool ShowVolumes, bool Opened)
{
  mShowVolumes = ShowVolumes;
  mpMainBox = new nuiHBox(3);
  mpMainBox->SetExpand(nuiExpandShrinkAndGrow);
  mpMainBox->SetCellExpand(1, nuiExpandShrinkAndGrow);
  
  AddChild(mpMainBox);
  
  if (mShowVolumes)
  {
    mpInfoBox = new nuiVBox(1);
    mpInfoBox->SetExpand(nuiExpandShrinkAndGrow);
    mpMainBox->SetCell(0, mpInfoBox);
    mpMainBox->SetCellMinPixels(0, 200);

    
    mpFolderList = new nuiList(nuiVertical);
    mpFolderList->SetObjectName(_T("nuiFileSelector::FolderView"));
    mpFolderList->SetObjectClass(_T("nuiFileSelector::FolderView"));
    mpInfoBox->SetCell(0, mpFolderList);    
    mpInfoBox->SetCellExpand(0, nuiExpandShrinkAndGrow);


    // list of volumes
    mVolumes.clear();
    nglPath::GetVolumes(mVolumes, nglPathVolume::All);
    
    std::list<nglPathVolume>::iterator it;
    for (it = mVolumes.begin(); it != mVolumes.end(); ++it)
    {
      nglPathVolume volume = *it;
    
      nuiHBox* pBox = new nuiHBox(0);
      pBox->SetToken(new nuiToken<nglString>(volume.mPath.GetPathName()));
      
      nuiSimpleContainer* pIcon = new nuiSimpleContainer();
      pIcon->SetObjectName(_T("nuiFileSelector::VolumeIcon"));
      pIcon->SetObjectClass(_T("nuiFileSelector::VolumeIcon"));
      pBox->AddCell(pIcon, nuiCenter);
      
      nuiLabel* pLabel = new nuiLabel(volume.mComment);
      pBox->AddCell(pLabel);
      pBox->SetCellExpand(pBox->GetNbCells()-1, nuiExpandShrinkAndGrow);
      pLabel->SetObjectName(_T("nuiFileSelector::FolderLabel"));
      pLabel->SetObjectClass(_T("nuiFileSelector::FolderLabel"));

      mpFolderList->AddChild(pBox);
    }

    
    nuiLabel* pSeparator = new nuiLabel(_T(" "));
    pSeparator->SetBorder(0, 0.5);
    pSeparator->SetEnabled(false);
    mpFolderList->AddChild(pSeparator);
    

    // list of user's favorites paths
    nglPath UserPath(ePathUser);
    nglPath DocPath(ePathUserDocuments);
    nglPath DesktopPath(ePathUserDesktop);
    nglPath paths[3] = {UserPath, DocPath, DesktopPath};
    
    for(int i = 0; i < 3; i++)
    {
      nuiHBox* pBox = new nuiHBox(0);
      pBox->SetToken(new nuiToken<nglString>(paths[i].GetPathName()));
      
      nuiSimpleContainer* pIcon = new nuiSimpleContainer();
      pIcon->SetObjectName(_T("nuiFileSelector::FolderIcon"));
      pIcon->SetObjectClass(_T("nuiFileSelector::FolderIcon"));
      pBox->AddCell(pIcon, nuiCenter);
    
      nuiLabel* pLabel = new nuiLabel(paths[i].GetNodeName());
      pBox->AddCell(pLabel);
      pBox->SetCellExpand(pBox->GetNbCells()-1, nuiExpandShrinkAndGrow);
      pLabel->SetObjectName(_T("nuiFileSelector::FolderLabel"));
      pLabel->SetObjectClass(_T("nuiFileSelector::FolderLabel"));
      mpFolderList->AddChild(pBox);
    }
    
    
    mEventSink.Connect(mpFolderList->SelectionChanged, &nuiFileSelector::OnFolderListSelectionChanged);
  }
  
    
  nuiTreeNode* pTree = new nuiTreeNode(new nuiLabel(_T("Loading...")));
  switch (mode)
  {
    case eTree:
      {
        mpTreeView = new nuiTreeView(pTree);
        mpTreeView->SetObjectName(_T("nuiFileSelector::TreeView"));
        mpTreeView->SetObjectClass(_T("nuiFileSelector::TreeView"));


        nuiScrollView* pScrollView = new nuiScrollView();
        pScrollView->AddChild(mpTreeView);
        mpMainBox->SetCell(1, pScrollView);
      }
      break;
    case eColumn:
      {
        nuiScrollView* pScrollView = new nuiScrollView(true, false);
        mpTreeView = new nuiColumnTreeView(pTree);
        mpTreeView->SetObjectName(_T("nuiFileSelector::TreeView"));
        mpTreeView->SetObjectClass(_T("nuiFileSelector::TreeView"));

        pScrollView->AddChild(mpTreeView);
        mpMainBox->SetCell(1, pScrollView);
      }
      break;
  }
  mDisplayMode = mode;
  
  mEventSink.Connect(mpTreeView->Activated, &nuiFileSelector::OnActivated);
	mEventSink.Connect(mpTreeView->SelectionChanged, &nuiFileSelector::OnSelected);
  
  mpEntry = pEntry;
  if (mpEntry)
  {
    mpEntry->Focus();
    mEventSink.Connect(mpEntry->Selected, &nuiFileSelector::OnActivated);
  }

  SetRootPath(rRootPath, Opened);
  SetPath(rPath);
}


void nuiFileSelector::SetInfoView(nuiWidget* pWidget)
{
  if (!mpInfoBox)
    return;
  
  if (!pWidget)
  {
    if (mpInfoBox->GetNbCells() > 1)
      mpInfoBox->RemoveCells(mpInfoBox->GetNbCells()-1, 1);
    return;
  }
  
  if (mpInfoBox->GetNbCells() == 2)
    mpInfoBox->SetCell(1, pWidget);
  else
    mpInfoBox->AddCell(pWidget);
  mpInfoBox->SetCellExpand(1, nuiExpandShrinkAndGrow);
}




void nuiFileSelector::FormatFileSize(nuiSize size, nglString& str)
{
  if (size > 1000000000)
    str.Format(_T("%.2f Go"), size / (1024*1024*1024));
  else if (size > 1000000)
    str.Format(_T("%.2f Mo"), size / (1024*1024));
  else if (size > 1000)
    str.Format(_T("%.2f Ko"), size / 1024);
  else 
    str.Format(_T("%d b"), size);  
}


nuiWidget* nuiFileSelector::GetFileInfo(const nglPath& rPath)
{
  nuiSimpleContainer* pMainCont = new nuiSimpleContainer();
  pMainCont->SetObjectName(_T("nuiFileSelector::InfoView"));
  pMainCont->SetObjectClass(_T("nuiFileSelector::InfoView"));
  
  nuiVBox* pBox = new nuiVBox(0);
  pBox->SetExpand(nuiExpandShrinkAndGrow);
  pMainCont->AddChild(pBox);
  
  // file icon
  nuiSimpleContainer* pIcon = new nuiSimpleContainer();
  pBox->AddCell(pIcon, nuiCenter);
  
  nglString objectName;
  objectName.Format(_T("nuiFileSelector::InfoViewIcon::%ls"), rPath.GetExtension().GetChars());
  pIcon->SetObjectName(objectName);
  pIcon->SetObjectClass(objectName);
  
  // file name
  nuiLabel* pLabel = new nuiLabel(rPath.GetNodeName());
  pLabel->UseEllipsis(true);
  pBox->AddCell(pLabel, nuiCenter);
  
  // file size
  nglPathInfo info;
  rPath.GetInfo(info);
  nglString sizestr;
  FormatFileSize(info.Size, sizestr);
  pBox->AddCell(new nuiLabel(sizestr), nuiCenter);
  
  // file modification date
  nglTimeInfo timeInfo;
  nglString timestr;
  nglTime lastMod = info.LastMod;
  lastMod.GetLocalTime(timeInfo);
  timestr.Format(_T("%d / %d / %d   %d:%d"), timeInfo.Year+1900, timeInfo.Month, timeInfo.Day, timeInfo.Hours, timeInfo.Minutes);
  pBox->AddCell(new nuiLabel(timestr), nuiCenter);

  
  return pMainCont;
}


bool nuiFileSelector::OnActivated (const nuiEvent& event)
{
  OK ();
  return true;
}



void nuiFileSelector::SetFileName (const nglString& FileName)
{
  if (mpEntry)
    mpEntry->SetText(FileName);
}




bool nuiFileSelector::SetPath(const nglPath& rPath)
{
  nglPath path(rPath);
  path.MakeRelativeTo(GetRootPath());
  std::vector<nglString> tokens;
  path.GetPathName().Tokenize(tokens, '/');
  nuiTreeNodePtr pNode = mpTreeView->GetTree();
  
  pNode->Select(false);
  nuiColumnTreeView* pColTreeView = dynamic_cast<nuiColumnTreeView*>(mpTreeView);
  
  for (uint i = 0; i < tokens.size(); i++)
  {
    pNode->OpenAllChildren(false);
    pNode->SelectAllChildren(false);
    
    std::vector<nuiTreePtr> children;
    pNode->GetChildren(children);
    for (uint j = 0; j< children.size(); j++)
    {
      nuiFileSelectorNode* pBNode = dynamic_cast<nuiFileSelectorNode*>(children.at(j));
      if (pBNode)
      {
        nglPath p(pBNode->GetProperty(_T("Path")));
        if (p.GetNodeName() == tokens.at(i))
        {
          pNode = pBNode;
          break;
        }
      }
      
    }
    
    if (pNode == mpTreeView->GetSelectedNode()) // Have we found the next node?
      return false; // No next node found: bail out...
        
    nuiTreeNodePtr pParent = pNode;
    while (pParent)
    {
      if (!pParent->IsEmpty())
        pParent->Open(true);
      if (pColTreeView)
        pParent->Select(true);
      pParent = static_cast<nuiTreeNodePtr>(pParent->GetParent());
    }

    if (pColTreeView)
      mpTreeView->Select(pNode, true);

    if (pNode->IsEmpty()) // Bail out if the last node is a leaf
    {
      PathChanged();
      mpTreeView->SelectionChanged();
      mpTreeView->SetRect(mpTreeView->GetIdealRect());
      if (pColTreeView)
      {
//        pColTreeView->CalcHotRect();
//        pColTreeView->CalcVerticalHotRect();
      }
      return false;
    }
    
//    if (pColTreeView)
//      pColTreeView->CreateScrollBars();
  }
  
//  if (!pColTreeView)
//  {
//    mpTreeView->Select(pNode, true);
//  }
  
  PathChanged();
  mpTreeView->SelectionChanged();
  mpTreeView->SetRect(mpTreeView->GetIdealRect());
  if (pColTreeView)
  {
//    pColTreeView->CalcHotRect();
//    pColTreeView->CalcVerticalHotRect();
  }  
  
  
  // Update infos and change the edit line:
  nglPathInfo info;
  rPath.GetInfo(info);

  if (info.Exists && info.IsLeaf)
    SetFileName(rPath.GetNodeName());
  
  mpTreeView->SetHotRect(mpTreeView->GetHotRect());

  return true;
}

bool nuiFileSelector::SetRootPath(const nglPath& rPath, bool Opened)
{
  nuiTreeNodePtr pNode = GetNewNode(rPath);

  if (pNode)
  {
    pNode->Open(Opened);
    mpTreeView->SetTree(pNode);
    SetPath(rPath);
  }
  return true;
}

nglPath nuiFileSelector::GetPath() const
{
  nuiTreeNode* pTree = mpTreeView->GetSelectedNode();
  if (!pTree)
    pTree = mpTreeView->GetTree();
	nglPath pathName = pTree->GetProperty(_T("Path"));
	nglString node = pathName.GetNodeName();
	
	nglString addin;
	if (mpEntry)
		addin = mpEntry->GetText();
	
	if ((addin != nglString::Null) && (node.Compare(addin)))
	{
		if (pathName.IsLeaf())
			return pathName.GetParent() + addin;
			
		return pathName + addin;
	}
	
	return pathName;
}

nglPath nuiFileSelector::GetFolderPath() const
{
  nglPath path = GetPath();
  
  if (!path.Exists() || path.IsLeaf())
  {
    // a file is selected, or the user already entered something in the entry box, get the parent.
    path = path.GetParent();
  }
  
  return path;
}

nglPath nuiFileSelector::GetRootPath() const
{
  nuiTreeNode* pTree = mpTreeView->GetTree();
  return pTree->GetProperty(_T("Path"));
}


nuiTreeNode* nuiFileSelector::GetRootNode()
{
  nuiTreeNode* pNode = mpTreeView->GetTree();
  return pNode;
}


bool nuiFileSelector::IsFilterSet(const nglString& rFilter)
{
  std::list<nglString>::iterator it;
  for (it = mFilters.begin(); it != mFilters.end(); ++it)
  {
    const nglString& filter = *it;
    if (!filter.Compare(rFilter))
      return true;
  }
  return false;
}

bool nuiFileSelector::IsFileFiltered(const nglPath& rFile)
{
  const nglString& ext = rFile.GetExtension();
  std::list<nglString>::iterator it;
  bool filterres =  false;
  for (it = mFilters.begin(); it != mFilters.end(); ++it)
  {
    const nglString& filter = *it;
    if (!filter.Compare(ext))
    {
      filterres = true;
      break;
    }
  }
  
  bool hiddenres = mShowHiddenFiles || (rFile.GetPathName().GetChar(0) != _T('.'));
  
  return (filterres && hiddenres);
}



nuiTreeNode* nuiFileSelector::GetNewNode(const nglPath& rPath)
{
  nuiWidget* pWidget= NULL;
  nuiHBox* pBox = NULL;
  nuiSimpleContainer* pIcon = NULL;
  nuiLabel* label = NULL;
  
  nglPath pathName = rPath;
  
    if (pathName.IsLeaf() || !pathName.GetExtension().Compare(_T("app")))
    {
      // no file is filtered
      if (mFilters.size() == 0)
        return NULL;
      
      else if (IsFilterSet(_T("*")))
      {
        if (!pathName.GetExtension().Compare(_T("app")))
          pathName = pathName.GetRemovedExtension();
        
        pBox = new nuiHBox(2);
        pIcon = new nuiSimpleContainer();
        pIcon->SetObjectName(_T("nuiFileSelector::TreeFileIcon"));
        pIcon->SetObjectClass(_T("nuiFileSelector::TreeFileIcon"));
        label = new nuiLabel(pathName.GetNodeName().IsEmpty()?_T("/"):pathName.GetNodeName());        
        label->SetObjectName(_T("nuiFileSelector::TreeFileLabel"));      
        label->SetObjectClass(_T("nuiFileSelector::TreeFileLabel"));      
      }
      
      else 
      {
        if (!IsFileFiltered(rPath.GetNodeName()))
          return NULL;
        else
        {
          if (!pathName.GetExtension().Compare(_T("app")))
            pathName = pathName.GetRemovedExtension();

          
          pBox = new nuiHBox(2);
          pIcon = new nuiSimpleContainer();
          nglString objectName;
          objectName.Format(_T("nuiFileSelector::TreeFilterIcon::%ls"), rPath.GetExtension().GetChars());
          pIcon->SetObjectName(objectName);
          pIcon->SetObjectClass(objectName);
          label = new nuiLabel(pathName.GetNodeName().IsEmpty()?_T("/"):pathName.GetNodeName());        
          label->SetObjectName(_T("nuiFileSelector::TreeFileLabel"));
          label->SetObjectClass(_T("nuiFileSelector::TreeFileLabel"));
        }
      }
      


    }
    else
    {
      if (!mShowHiddenFiles && (rPath.GetNodeName().GetChar(0) == _T('.')))
        return NULL;
        
      nglString iconObjectName, labelObjectName;
      
      if (rPath.GetParent().GetPathName().IsEmpty() || !rPath.GetPathName().Compare(_T("/")) || !rPath.GetParent().GetPathName().Compare(_T("/Volumes")))
      {
        iconObjectName = _T("nuiFileSelector::VolumeIcon");
        labelObjectName = _T("nuiFileSelector::TreeFolderLabel");
      }
      else
      {
        iconObjectName = _T("nuiFileSelector::TreeFolderIcon");
        labelObjectName = _T("nuiFileSelector::TreeFolderLabel");
      }
      
      
      pBox = new nuiHBox(2);
      pIcon = new nuiSimpleContainer();
      pIcon->SetObjectName(iconObjectName);
      pIcon->SetObjectClass(iconObjectName);
      label = new nuiLabel(rPath.GetNodeName().IsEmpty()?_T("/"):rPath.GetNodeName());        
      label->SetObjectName(labelObjectName);
      label->SetObjectClass(labelObjectName);
    }
    
    pBox->SetCell(0, pIcon, nuiCenter);
    pBox->SetCell(1, label);
    
    pWidget = pBox;
  
  if (!pWidget)
    return NULL;
    
  nuiFileSelectorNode* pNewNode = new nuiFileSelectorNode(this, rPath, pWidget);
  
  return pNewNode;
}

void nuiFileSelector::SetDisplayMode(nuiFileSelector::DisplayMode mode)
{
  if (mDisplayMode == mode)
    return;
  
  mDisplayMode = mode;
  nuiTreeNodePtr pTree = mpTreeView->GetTree();
  mpTreeView->SetTree(new nuiTreeNode(_T("Changing view mode...")), false);
  
  Clear();
  
  switch (mode)
  {
    case eTree:
      {
        mpTreeView = new nuiTreeView(pTree);
        mpTreeView->SetObjectName(_T("nuiFileSelector::TreeView"));
        mpTreeView->SetObjectClass(_T("nuiFileSelector::TreeView"));

        nuiScrollView* pScrollView = new nuiScrollView();
        pScrollView->AddChild(mpTreeView);
        mpMainBox->SetCell(1, pScrollView);
//        AddChild(pScrollView);
      }
      break;
    case eColumn:
      {
        mpTreeView = new nuiColumnTreeView(pTree);
        mpTreeView->SetObjectName(_T("nuiFileSelector::TreeView"));
        mpTreeView->SetObjectClass(_T("nuiFileSelector::TreeView"));

        nuiScrollView* pScrollView = new nuiScrollView(true, false);
        pScrollView->AddChild(mpTreeView);
        mpMainBox->SetCell(1, pScrollView);
//        AddChild(pScrollView);
      }
      break;
  }
}

nuiFileSelector::DisplayMode nuiFileSelector::GetDisplayMode() const
{
  return mDisplayMode;
}






bool nuiFileSelector::OnSelected (const nuiEvent& rEvent)
{
	nglPath curPath;
  nuiTreeNode* pTree = mpTreeView->GetSelectedNode();
	if (pTree != NULL)
		curPath = pTree->GetProperty(_T("Path"));
  
   
	if (curPath.IsLeaf())
  {
		nglString addin = curPath.GetNodeName();
    
    if (IsFileFiltered(addin))
      SetInfoView(GetFileInfo(curPath));
    else
      SetInfoView(NULL);
    
    if (mpEntry == NULL)
      return false;
    
    mpEntry->SetText(addin);
  }
  else
    SetInfoView(NULL);
	
	return false;
}
  
bool nuiFileSelector::OnFolderListSelectionChanged(const nuiEvent& event)
{
  if(!mShowVolumes)
    return false;
  nuiToken<nglString>* pToken = (nuiToken<nglString>*)(mpFolderList->GetSelectedToken());
  nglString SelectedPathName;
  nuiGetTokenValue<nglString>(pToken, SelectedPathName);
  nglPath SelectedPath(SelectedPathName);
  NGL_OUT(_T("new Path :__") + SelectedPath.GetPathName() + _T("__\n"));
  SetRootPath(SelectedPath);
  return false;
}





