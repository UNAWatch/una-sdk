#ifndef TITLE_HPP
#define TITLE_HPP

#include <gui_generated/containers/TitleBase.hpp>

class Title : public TitleBase
{
public:
    Title();
    virtual ~Title() {}

    virtual void initialize();

    void set(TypedTextId msgId);
    void set(touchgfx::Unicode::UnicodeChar *msg);
protected:
};

#endif // TITLE_HPP
