#pragma once
#include <memory>

#include "CVETypes.h"

class CVEModel;

class CVEGameObject
{
public:
    CVEGameObject(const CVETransform& transform = CVETransform());
    CVEGameObject(const std::shared_ptr<CVEModel>& model, const CVETransform& transform = CVETransform());
    ~CVEGameObject();
    
    CVEGameObject(const CVEGameObject&) = delete;
    CVEGameObject& operator=(const CVEGameObject&) = delete;
    CVEGameObject(CVEGameObject&&) = default;
    CVEGameObject& operator=(CVEGameObject&&) = delete;
    
    CVETransform Transform;
    
private:
    std::shared_ptr<CVEModel> Model;
};
