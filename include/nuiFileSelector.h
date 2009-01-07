/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

/*!
\file  nuiFileSelector.h
\brief File selector
*/

#ifndef __nuiFileSelector_h__
#define __nuiFileSelector_h__

#include "nuiComposite.h"
#include "nuiEditLine.h"
#include "nuiTreeView.h"
#include "nuiColumnTreeView.h"
#include "nglVolume.h"
#include "nuiHBox.h"
#include "nuiVBox.h"
#include "nuiList.h"


//! Simple file selector
/*
This composite widget does the usual combination of a file browser (based on nuiFileTable),
an entry (a nuiEditLine)

NOTE ABOUT DECORATIONS:

Object names have been assigned to the nuiFileSelector children, 
in order to get an easy way to set a decoration within a css file.
You just have to use the following labels as object names in your css file.

"nuiFileSelector::FolderView"   : the volumes and folders view widget on the left side of the selector
"nuiFileSelector::InfoView"
"nuiFileSelector::FolderLabel"  : the volumes and folders label widgets
"nuiFileSelector::VolumeIcon"   : icon for the volumes items
"nuiFileSelector::FolderIcon"   : icon for the folders items
"nuiFileSelector::InfoViewIcon::[FILTER]"
 
 
"nuiFileSelector::TreeView"         : the folders and files tree view on the right side of the selector
"nuiFileSelector::TreeFolderLabel"  : the folders label widgets 
"nuiFileSelector::TreeFileLabel"    : the files label widgets
"nuiFileSelector::TreeFolderIcon"   : icon for the folders items
"nuiFileSelector::TreeFileIcon"     : icon for the files items
"nuiFileSelector::TreeFilterIcon::[FILTER]" : dynamically generated name, icon for the filtered files, when a filter is given as a parameter to the nuiFileSelector. For instance :  "nuiFileSelector::TreeFilterIcon::txt"

*/


class NUI_API nuiFileSelector : public nuiComposite
{
public:
  enum DisplayMode
  {
    eTree,
    eColumn
  };
  
  nuiFileSelector(const nglPath& rPath, const nglPath& rRootPath, const nglString& rFilter = _T("*"), nuiEditLine* pEntry = NULL, bool showHiddenFiles = false, DisplayMode mode = eColumn, bool ShowVolumes = true, bool Opened = true);
  nuiFileSelector(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry = NULL, bool showHiddenFiles = false, DisplayMode mode = eColumn, bool ShowVolumes = true, bool Opened = true);
  nuiFileSelector(const nglPath& rPath, const nglString& rFilter = _T("*"), nuiEditLine* pEntry = NULL, bool showHiddenFiles = false, DisplayMode mode = eColumn, bool ShowVolumes = true, bool Opened = true);
  nuiFileSelector(const nglPath& rPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry = NULL, bool showHiddenFiles = false, DisplayMode mode = eColumn, bool ShowVolumes = true, bool Opened = true);
  /*!< Creates a file selector instance
    \param pParent container
    \param rPath initial path displayed by the file browser
  */
  
  void InitAttributes();
  
  virtual ~nuiFileSelector();
  
  void InitSelector(const nglPath& rPath, const nglPath& rRootPath, nuiEditLine* pEntry = NULL, DisplayMode mode = eColumn, bool ShowVolumes = true, bool Opened = true);

  void SetFileName(const nglString& FileName);  ///< Set the entry widget's current text

  bool SetPath(const nglPath& rPath);      ///< Change currently selected Path.
  bool SetRootPath(const nglPath& rPath, bool Opened = true);      ///< Change current root
  nglPath GetPath() const;          ///< Retrieve currently selected Path, including the node currently entered in the entry box
  nglPath GetFolderPath() const; ///< Retrieve currently selected Path, or its parent if a file is selected. The entry box content is not used.
  nglPath GetRootPath() const;          ///< Retrieve current root.
  nuiTreeNode* GetRootNode();

  void SetDisplayMode(DisplayMode mode);
  DisplayMode GetDisplayMode() const;
  
  nuiSimpleEventSource<nuiWidgetActivated> PathChanged;      ///< Event triggered when the user is navigating around the file system.
  nuiSimpleEventSource<nuiWidgetActivated> OK;      ///< Event triggered when the user was done (file selected)
  nuiSimpleEventSource<nuiActionAborted>   Cancel;  ///< Event triggered when the user was done (closed/cancelled)
  
private:

  void Init(const nglPath& rPath, const nglPath& rRootPath, const std::list<nglString>& rFilters, nuiEditLine* pEntry, bool showHiddenFiles, DisplayMode mode, bool ShowVolumes, bool Opened);
  
  void SetInfoView(nuiWidget* pWidget);
  nuiWidget* GetFileInfo(const nglPath& rPath);
  void FormatFileSize(nuiSize size, nglString& str);

  
  bool IsFilterSet(const nglString& rFilter);
  bool IsFileFiltered(const nglPath& rFile);
  
  // attributes
  const std::list<nglString>& GetFilters();
  void SetFilters(const std::list<nglString>& rFilters);
  nglString GetLastFilter();
  void AddFilter(nglString filter);
  std::list<nglString> mFilters;
  
  bool mShowHiddenFiles;

  virtual nuiTreeNode* GetNewNode(const nglPath& rPath);
  bool OnActivated (const nuiEvent& event);
  bool OnFolderListSelectionChanged(const nuiEvent& event);

  nuiHBox* mpMainBox;
  nuiVBox* mpInfoBox;
  nuiEditLine* mpEntry;
  nuiTreeView* mpTreeView;
  nuiList* mpFolderList;
  nglPath mSelection;
  bool mShowVolumes;
  std::list<nglPathVolume> mVolumes;

  bool OnClosed   (const nuiEvent& rEvent);
  bool OnOK       (const nuiEvent& rEvent);
  bool OnSelected (const nuiEvent& rEvent);
  
  friend class nuiFileSelectorNode;
  
  DisplayMode mDisplayMode;
  nuiEventSink<nuiFileSelector> mEventSink;
  
};

#endif // __nuiFileSelector_h__
