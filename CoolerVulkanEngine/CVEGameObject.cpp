#include "CVEGameObject.h"

CVEGameObject::CVEGameObject(const CVETransform& transform)
    : Transform(transform)
{
}

CVEGameObject::CVEGameObject(const std::shared_ptr<CVEModel>& model, const CVETransform& transform)
    : Transform(transform)
    , Model(model)
{
}

CVEGameObject::~CVEGameObject()
{
}
