#include "commit.h"
#include "strings.h"
bool getBranchName(String const s);
bool getCurrentIndexFile(String s);
bool copyFileToCache(File f);
bool copyTreeToRepo(File f);
bool getCurrentCommit(Commit c, ShaBuffer currentCommit);
