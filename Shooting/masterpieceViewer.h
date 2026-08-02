// masterpieceViewer.h

#pragma once
#include <vector>
#include <string>

struct MasterpieceEntry {
    std::string stageId;
    std::string comment;
};

extern std::vector<MasterpieceEntry> masterpieceList;

extern bool masterpieceMode;
extern const char* g_masterpieceComment;

int masterpieceMain();