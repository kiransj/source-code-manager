#include "commit.h"
#include "strings.h"
bool getBranchName(String const s);
bool getCurrentIndexFile(String s);
bool copyFileToCache(File f);
bool copyFileToRepo(File f);
bool copyFileFromRepo(File f);

bool copyTreeToRepo(File f);
bool copyTreeFromRepo(ShaBuffer tree, const char *dest, int mode);

bool getCurrentCommit(Commit c, ShaBuffer currentCommit);
bool setCurrentCommit(ShaBuffer commitSha);
bool compareIndexWithWorkingArea(void);
