#include "commit.h"
#include "strings.h"
bool getBranchName(String const s);
bool getCurrentIndexFile(String s);
bool copyFileToCache(File f);
bool copyFileToRepo(File f);
bool copyTreeToRepo(File f);
bool getCurrentCommit(Commit c, ShaBuffer currentCommit);
bool setCurrentCommit(ShaBuffer commitSha);

