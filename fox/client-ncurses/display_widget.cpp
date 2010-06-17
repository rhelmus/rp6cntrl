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
}

void CDisplayWidget::addDisplay(const std::string &l, int id, int amount)
{
    keyBox->StartPack(new NNCurses::CLabel(l, false), false, false, 0, 0);

    NNCurses::CBox *hbox = new NNCurses::CBox(CBox::HORIZONTAL, false);
    dataBox->StartPack(hbox, true, true, 0, 0);
    
    for (int i=0; i<amount; ++i)
    {
        if (i > 0) // Add seperator?
            hbox->StartPack(new NNCurses::CLabel("/"), true, true, 0, 0);
        
        NNCurses::CLabel *label = new NNCurses::CLabel("NA");
        label->SetDFColors(COLOR_YELLOW, COLOR_BLUE);
        hbox->StartPack(label, true, true, 0, 0);
        displayValueMaps[id].push_back(label);
    }
}

void CDisplayWidget::setDisplayValue(int id, int index, const std::string &value)
{
    displayValueMaps[id][index]->SetText(value);
}
