#include <gui/rect.h>
#include <math.h>
#include <log.h>
#include <heap.h>

using namespace LIBCactusOS;

Rectangle::Rectangle(int w, int h, int x_p, int y_p)
{
    this->width = w;
    this->height = h;
    this->x = x_p;
    this->y = y_p;
}
Rectangle::Rectangle()
{
    this->width = 0;
    this->height = 0;
    this->x = 0;
    this->y = 0;
}
int Rectangle::Area()
{
    return this->width * this->height;
}
bool Rectangle::Intersect(Rectangle other, Rectangle* result)
{
    int top = Math::Max(this->y, other.y);
    int bottom = Math::Min(this->y + this->height, other.y + other.height);
    int left = Math::Max(this->x, other.x);
    int right = Math::Min(this->x + this->width, other.x + other.width);

    if(result) {
        result->x = left;
        result->y = top;
        result->width = right - left;
        result->height = bottom - top;
    }

    return (top < bottom && left < right);
}

bool Rectangle::Contains(int x, int y)
{
    if(x >= this->x && x <= this->x + this->width)
        if(y >= this->y && y <= this->y + this->height)
            return true;
    return false;
}

// Makes cutting a lot easier
typedef struct Rect_struct {
    int top;
    int left;
    int bottom;
    int right;
} Rect;

///////////
// Mostly from http://www.trackze.ro/wsbe-4-get-clippy/
///////////

List<Rectangle>* Rectangle::Split(Rectangle cutter, List<Rectangle>* output)
{
    //Allocate the list of result rectangles
    List<Rectangle>* output_rects = output != 0 ? output : new List<Rectangle>();

    //We're going to modify the subject rect as we go,
    //so we'll clone it so as to not upset the object 
    //we were passed
    Rect subject_copy;
    subject_copy.top = this->y;
    subject_copy.left = this->x;
    subject_copy.bottom = this->y + this->height;
    subject_copy.right = this->x + this->width;

    Rect cutting_rect;
    cutting_rect.top = cutter.y;
    cutting_rect.left = cutter.x;
    cutting_rect.bottom = cutter.y + cutter.height;
    cutting_rect.right = cutter.x + cutter.width;

    //Begin splitting
    //1 -Split by left edge if that edge is between the subject's left and right edges 
    if(cutting_rect.left >= subject_copy.left && cutting_rect.left <= subject_copy.right) {
        Rect temp_rect;
        temp_rect.top = subject_copy.top;
        temp_rect.left = subject_copy.left;
        temp_rect.bottom = subject_copy.bottom;
        temp_rect.right = cutting_rect.left - 1;

        //Add the new rectangle to the output list
        output_rects->push_back(Rectangle(temp_rect.right - temp_rect.left, temp_rect.bottom - temp_rect.top, temp_rect.left, temp_rect.top));

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.left = cutting_rect.left;
    }

    //2 -Split by top edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect.top >= subject_copy.top && cutting_rect.top <= subject_copy.bottom) {
        Rect temp_rect;
        temp_rect.top = subject_copy.top;
        temp_rect.left = subject_copy.left;
        temp_rect.bottom = cutting_rect.top - 1;
        temp_rect.right = subject_copy.right;

        //Add the new rectangle to the output list
        output_rects->push_back(Rectangle(temp_rect.right - temp_rect.left, temp_rect.bottom - temp_rect.top, temp_rect.left, temp_rect.top));

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.top = cutting_rect.top;
    }

    //3 -Split by right edge if that edge is between the subject's left and right edges 
    if(cutting_rect.right >= subject_copy.left && cutting_rect.right <= subject_copy.right) {
        Rect temp_rect;
        temp_rect.top = subject_copy.top;
        temp_rect.left = cutting_rect.right + 1;
        temp_rect.bottom = subject_copy.bottom;
        temp_rect.right = subject_copy.right;

        //Add the new rectangle to the output list
        output_rects->push_back(Rectangle(temp_rect.right - temp_rect.left, temp_rect.bottom - temp_rect.top, temp_rect.left, temp_rect.top));

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.right = cutting_rect.right;
    }

    //4 -Split by bottom edge if that edge is between the subject's top and bottom edges 
    if(cutting_rect.bottom >= subject_copy.top && cutting_rect.bottom <= subject_copy.bottom) {
        Rect temp_rect;
        temp_rect.top = cutting_rect.bottom + 1;
        temp_rect.left = subject_copy.left;
        temp_rect.bottom = subject_copy.bottom;
        temp_rect.right = subject_copy.right;

        //Add the new rectangle to the output list
        output_rects->push_back(Rectangle(temp_rect.right - temp_rect.left, temp_rect.bottom - temp_rect.top, temp_rect.left, temp_rect.top));

        //Shrink the subject rectangle to exclude the split portion
        subject_copy.bottom = cutting_rect.bottom;
    }
 
    //Finally, after all that, we can return the output rectangles 
    return output_rects;
}
void Rectangle::PushToClipList(List<Rectangle>* targetList)
{
    int i;
    List<Rectangle>* split_rects;

    Rect subject_copy;
    subject_copy.top = this->y;
    subject_copy.left = this->x;
    subject_copy.bottom = this->y + this->height;
    subject_copy.right = this->x + this->width;

    //Check each item already in the list to see if it overlaps with
    //the new rectangle
    for(i = 0; i < targetList->size(); ) {
        Rectangle rect = targetList->GetAt(i);
        Rect cur_rect;
        cur_rect.top = rect.y;
        cur_rect.left = rect.x;
        cur_rect.bottom = rect.y + rect.height;
        cur_rect.right = rect.x + rect.width;

        //Standard rect intersect test
        //see here for an example of why this works:
        //http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other#tab-top
        if(!(cur_rect.left <= subject_copy.right &&
		   cur_rect.right >= subject_copy.left &&
		   cur_rect.top <= subject_copy.bottom &&
		   cur_rect.bottom >= subject_copy.top)) {

            //If this rect doesn't intersect with the added_rect
            //then we can just move on to the next one
            i++;
            continue;
        }

        //If this rectangle *does* intersect with the new rectangle, 
        //we need to split it
        targetList->Remove(i);
        split_rects = rect.Split(*this); //Do the split

        //Copy the split, non-overlapping result rectangles into the list 
        while(split_rects->size()) {
            rect = split_rects->GetAt(0);
            split_rects->Remove(0);
            targetList->push_back(rect); //Push to B
        }

        //Free the split_rect list that we just emptied
        delete split_rects;

        //Since we removed an item from the list, we need to start counting over again 
        //In this way, we'll only exit this loop once nothing in the list overlaps 
        i = 0;    
    }

    //Now that we have made sure none of the existing rectangles overlap
    //with the new rectangle, we can finally insert it into the hole
    //we just created
    targetList->push_back(*this);
}
Rectangle Rectangle::Zero()
{
    return Rectangle(0, 0, 0, 0);
}