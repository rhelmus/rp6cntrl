#include "tui/box.h"

namespace NNCurses {
class CLabel;
}

class CDisplayWidget: public NNCurses::CBox
{
    NNCurses::CBox *keyBox, *dataBox;

public:
    CDisplayWidget(const std::string &t);

    NNCurses::CLabel *addDisplay(const std::string &l);
};
