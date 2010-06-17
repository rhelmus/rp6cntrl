#include <map>

#include "tui/box.h"

namespace NNCurses {
class CLabel;
}

class CDisplayWidget: public NNCurses::CBox
{
    NNCurses::CBox *keyBox, *dataBox;
    std::map<int, std::vector<NNCurses::CLabel *> > displayValueMaps;

public:
    CDisplayWidget(const std::string &t);

    void addDisplay(const std::string &l, int id, int amount);
    void setDisplayValue(int id, int index, const std::string &value);
};
