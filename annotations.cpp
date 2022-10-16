#include "annotations.h"
#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "fsm.h"
#include <fstream>
#include "imgui.h"

Annotation::Annotation(std::string label)
{
    // set attributes
    this->label = label;
    this->type = ANNOTATION_TYPE_POINT;

    color[3] = 1.0;
    for (auto n = 0; n < 3; n++)
    {
        this->color[n] = (float)std::rand() / RAND_MAX;
    }

    strcpy(this->new_label, this->label.c_str());
}

void Annotation::update_color(void)
{
    for (long unsigned int n = 0; n < this->inst.size(); n++)
    {
        this->inst[n].set_color(this->color);
    }
}

// -- INSTANCES
AnnotationInstance::AnnotationInstance(void)
{
    // finite state machine instanciation
    this->fsm.add_transitions({
        {States::CREATE, States::IDLE, "from_create_to_idle", nullptr, nullptr},
        {States::IDLE, States::EDIT, "from_idle_to_edit", nullptr, nullptr},
        {States::EDIT, States::IDLE, "from_edit_to_idle", nullptr, nullptr},
    });

    this->outer_rect = Rectangle(ImVec2(0, 0), ImVec2(0, 0));
    this->inner_rect = Rectangle(ImVec2(0, 0), ImVec2(0, 0));
    this->delta = 10.0;
    this->selected = false;
}

void AnnotationInstance::update_bounding_box(void)
{
    this->outer_rect.center.x = std::abs(this->coords[1].x + this->coords[0].x) / 2.0;
    this->outer_rect.center.y = std::abs(this->coords[1].y + this->coords[0].y) / 2.0;
    this->outer_rect.span.x = std::abs(this->coords[1].x - this->coords[0].x) + this->delta;
    this->outer_rect.span.y = std::abs(this->coords[1].y - this->coords[0].y) + this->delta;

    this->inner_rect.center = this->outer_rect.center;
    this->inner_rect.span.x = std::max(1, (int) std::abs(this->coords[1].x - this->coords[0].x) - this->delta);
    this->inner_rect.span.y = std::max(1, (int) std::abs(this->coords[1].y - this->coords[0].y) - this->delta);
}

void AnnotationInstance::set_fname(std::string fname)
{
    this->img_fname = fname;
}

void AnnotationInstance::set_color(float color[4])
{
    for (int k = 0; k < 4; k++)
        this->color_u8[k] = color[k] * 255;
}

void AnnotationInstance::set_corner_start(ImVec2 pos)
{
    this->coords[0] = pos;
}

void AnnotationInstance::set_corner_end(ImVec2 pos)
{
    coords[1].x = std::max(coords[0].x, pos.x);
    coords[1].y = std::max(coords[0].y, pos.y);
    coords[0].x = std::min(coords[0].x, pos.x);
    coords[0].y = std::min(coords[0].y, pos.y);
}

void AnnotationInstance::draw(void)
{
    // todo : color memed as IM_COL32 as well as floats
    // todo : store those absolute coordinates for optimisation?

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 _w = ImGui::GetWindowPos();
    ImVec2 _m = ImGui::GetMousePos();

    // compute absolute coodinates
    ImVec2 start = _w;
    start.x += this->coords[0].x;
    start.y += this->coords[0].y;

    if (this->fsm.state() == States::CREATE)
    {
        draw_list->AddRect(start, _m, IM_COL32(this->color_u8[0], this->color_u8[1], this->color_u8[2], this->color_u8[3]), 0.0, 0, 2.0);
    }
    else if (this->fsm.state() == States::IDLE)
    {
        // position on screen
        ImVec2 end = ImVec2(this->coords[1].x + _w.x, this->coords[1].y + _w.y);

        // position on image
        ImVec2 _pos = ImVec2(_m.x - _w.x, _m.y - _w.y);

        // change thickness if hovered
        float _thickness = 1.0;
        if (outer_rect.inside(_pos) && !inner_rect.inside(_pos))
        {
            _thickness = 2.0;
        }

        draw_list->AddRect(start, end, IM_COL32(this->color_u8[0], this->color_u8[1], this->color_u8[2], this->color_u8[3]), 0.0, 0, _thickness);
    }
}

Rectangle::Rectangle(void)
{
}

Rectangle::Rectangle(ImVec2 start, ImVec2 end)
{
    this->span.x = std::abs(end.x - start.x);
    this->span.y = std::abs(end.y - start.y);

    if (start.x < end.x)
        this->center.x = start.x + 0.5 * this->span.x;
    else
        this->center.x = end.x + 0.5 * this->span.x;

    if (start.y < end.y)
        this->center.y = start.y + 0.5 * this->span.y;
    else
        this->center.y = end.y + 0.5 * this->span.y;
}

bool Rectangle::intersect(Rectangle rect)
{
    if ((std::abs(center.x - rect.center.x) < 0.5 * (span.x + rect.span.x)) && (std::abs(center.y - rect.center.y) < 0.5 * (span.y + rect.span.y)))
    {
        return true;
    }
    return false;
}

bool Rectangle::inside(ImVec2 point)
{
    if ((2.0 * std::abs(point.x - center.x) < span.x) && (2.0 * std::abs(point.y - center.y) < span.y))
    {
        return true;
    }
    return false;
}
