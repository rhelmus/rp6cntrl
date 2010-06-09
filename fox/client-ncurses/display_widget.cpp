#include "tui/label.h"
#include "tui/separator.h"

#include "display_widget.h"

CDisplayWidget::CDisplayWidget(const std::string &t) :
                                    NNCurses::CBox(CBox::VERTICAL, false)
{
    SetBox(true);
    SetMinWidth(10);
    
    StartPack(new NNCurses::CLabel(t), false, true, 0, 0);
    StartPack(new NNCurses::CSeparator(ACS_HLINE), false, true, 0, 0);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, false, 1);
    StartPack(hbox, false, false, 0, 0);
    
    keyBox = new NNCurses::CBox(CBox::VERTICAL, false);
    hbox->StartPack(keyBox, false, false, 0, 0);
    
    dataBox = new NNCurses::CBox(CBox::VERTICAL, false);
    hbox->StartPack(dataBox, true, true, 0, 0);
    dataBox->SetDFColors(COLOR_YELLOW, COLOR_BLUE);
}

NNCurses::CLabel *CDisplayWidget::addDisplay(const std::string &l)
{
    keyBox->StartPack(new NNCurses::CLabel(l, false), false, false, 0, 0);

    NNCurses::CLabel *ret = new NNCurses::CLabel("NA");
    dataBox->StartPack(ret, true, true, 0, 0);

    return ret;
}
