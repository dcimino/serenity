#include "PaintableWidget.h"
#include <LibGUI/GPainter.h>
#include <SharedGraphics/GraphicsBitmap.h>

PaintableWidget::PaintableWidget(GWidget* parent)
    : GWidget(parent)
{
    set_fill_with_background_color(true);
    set_background_color(Color::LightGray);
    m_bitmap = GraphicsBitmap::create(GraphicsBitmap::Format::RGB32, { 600, 400 });
    m_bitmap->fill(Color::White);
}

PaintableWidget::~PaintableWidget()
{
}

void PaintableWidget::paint_event(GPaintEvent& event)
{
    GPainter painter(*this);
    painter.add_clip_rect(event.rect());
    painter.blit({ 0, 0 }, *m_bitmap, m_bitmap->rect());
}

void PaintableWidget::mousedown_event(GMouseEvent& event)
{
    if (event.button() != GMouseButton::Left)
        return;

    GPainter painter(*m_bitmap);
    painter.set_pixel(event.position(), Color::Black);
    update({ event.position(), { 1, 1 } });
}

void PaintableWidget::mouseup_event(GMouseEvent&)
{
}

void PaintableWidget::mousemove_event(GMouseEvent& event)
{
    if (!rect().contains(event.position()))
        return;

    if (event.buttons() & GMouseButton::Left) {
        GPainter painter(*m_bitmap);
        painter.set_pixel(event.position(), Color::Black);
        update({ event.position(), { 1, 1 } });
    }
}