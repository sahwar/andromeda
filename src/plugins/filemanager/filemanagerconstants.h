#ifndef FILEMANAGERCONSTANTS_H
#define FILEMANAGERCONSTANTS_H

namespace Constants {

namespace Menus {

const char * const SortBy = "SortBy";

} // namespace Menus

namespace MenuGroups {

const char * const FileInfo = "FileMenu 3";
const char * const FileChange = "FileMenu 4";

const char * const ViewViewMode = "ViewMenu 1";
const char * const ViewSortBy = "ViewMenu 2";
const char * const ViewSortByOrder = "ViewMenu 3";

const char * const Locations = "GotoMenu 1";

} // namespace Groups

namespace Actions {

const char * const Open = "Open";

const char * const FileInfo = "FileInfo";

const char * const NewFolder = "NewFolder";
const char * const CopyFiles = "CopyFiles";
const char * const MoveFiles = "MoveFiles";
const char * const Rename = "Rename";
const char * const Remove = "Remove";

const char * const IconMode = "IconMode";
const char * const ColumnMode = "ColumnMode";
const char * const TreeMode = "TreeViewMode";
const char * const CoverFlowMode = "CoverFlowMode";
const char * const DualPane = "DualPane";
const char * const VerticalPanels = "VerticalPanels";

const char * const ShowHiddenFiles = "ShowHiddenFiles";

const char * const SortByName = "SortByName";
const char * const SortBySize = "SortBySize";
const char * const SortByType = "SortByType";
const char * const SortByDate = "SortByDate";
const char * const SortByDescendingOrder = "SortByDescendingOrder";

const char * const Goto = "Goto %1";

} // namespace Actions

} // namespace Constants

#endif // FILEMANAGERCONSTANTS_H
