//
// Created by birb on 7/6/26.
//




#include "../logic/LoadAPWorld.h"
#include "APWorldWidget.h"


#include "APWorldDragAndDrop.h"

APWorldWidget::APWorldWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
    
}


void APWorldWidget::dropEvent(QDropEvent* event) {
    handleDropEvent(event);
}


void APWorldWidget::dragEnterEvent(QDragEnterEvent* event) {
    handleDragEnterEvent(event);
}
