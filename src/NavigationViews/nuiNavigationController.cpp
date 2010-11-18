/*
 NUI3 - C++ cross-platform GUI framework for OpenGL based applications
 Copyright (C) 2002-2003 Sebastien Metrot & Vincent Caron
 
 licence: see nui3/LICENCE.TXT
 */


#include "nui.h"
#include "nuiNavigationController.h"
#include "nuiViewController.h"
#include "nuiAttributeAnimation.h"


#define NOTIF_PENDING_OPERATION _T("PendingOp")


// static inits
//
std::vector<nuiEasingMethod> nuiNavigationController::mEasings;
std::vector<float> nuiNavigationController::mDurations;
//



nuiNavigationController::nuiNavigationController()
: nuiSimpleContainer(), mEventSink(this)
{
  if (SetObjectClass(_T("nuiNavigationController")))
    InitAttributes();
  
  if (mEasings.size() == 0)
    InitStatic();
  
  mpIn = NULL;
  mpOut = NULL;
  mPushed = false;
  mPoped = false;
  
  mPendingLayout = true;
 
}



nuiNavigationController::~nuiNavigationController()
{
  
}

void nuiNavigationController::InitStatic()
{
  mEasings.resize(eTransitionNone);
  mDurations.resize(eTransitionNone);
  
  mEasings[eTransitionSlide] = nuiEasingSinusStartFast;
  mDurations[eTransitionSlide] = 0.25f;
  
  mEasings[eTransitionElastic] = nuiEasingElasticOut<300>;
  mDurations[eTransitionElastic] = 0.8f;
}


void nuiNavigationController::InitAttributes()
{
  mAnimPosition = 0.f;
  AddAttribute(new nuiAttribute<nuiSize>
               (nglString(_T(".AnimPosition")), nuiUnitNone,
                nuiMakeDelegate(this, &nuiNavigationController::GetAnimPositon), 
                nuiMakeDelegate(this, &nuiNavigationController::SetAnimPosition)));  
}


void nuiNavigationController::SetAnimPosition(nuiSize value)
{
  mAnimPosition = value;
  UpdateLayout();
}


nuiSize nuiNavigationController::GetAnimPositon() const
{
  return mAnimPosition;
}

void nuiNavigationController::PushViewController(nuiViewController* pViewController, bool animated, TransitionType transition, bool viewOverlay)
{
  // store a pending operation and call for a asynchronous process
  mPendingOperations.push_back(PendingOperation(pViewController, ePendingPush, animated, transition, viewOverlay));
  GetTopLevel()->PostNotification(new nuiNotification(NOTIF_PENDING_OPERATION));
}


void nuiNavigationController::_PushViewController(nuiViewController* pViewController, bool animated, TransitionType transition, bool viewOverlay)
{
  // don't overlapp animations
  if (mPushed || mPoped)
  {
    mPendingOperations.push_back(PendingOperation(pViewController, ePendingPush, animated, transition, viewOverlay));
    return;
  }
  
  // store the push request if we're not connected to the top level yet
  if (mPendingLayout)
  {
    mPendingOperations.push_back(PendingOperation(pViewController, ePendingPush, animated, transition, viewOverlay));
    return;
  }
  
  pViewController->Acquire();
  mpIn = pViewController;
  mpOut  = NULL;
  
  mpIn->mAnimated = animated;
  if (mpOut)
    mpOut->mAnimated = animated;
  
  if ((mViewControllers.size() >0) && !viewOverlay)
    mpOut = mViewControllers.back();
  
  // push the new view in the stack
  mViewControllers.push_back(mpIn);
  
  nuiRect idealsize = GetRect().Size();
  
  
  // animation was requested. launch animation and connect the event
  if (animated && (transition != eTransitionNone))
  {
    mPushed = true;
    mPoped = false;
    
    mAnimPosition = idealsize.GetWidth();
    
    AddChild(mpIn);  
    
    nuiAttributeAnimation* pAnim = new nuiAttributeAnimation();
    pAnim->SetTargetObject(this);
    pAnim->SetTargetAttribute(_T(".AnimPosition"));
    pAnim->SetStartValue(idealsize.GetWidth());
    pAnim->SetEndValue(0);
    pAnim->SetEasing(mEasings[transition]);
    pAnim->SetDuration(mDurations[transition]);
    pAnim->SetDeleteOnStop(true);
    mEventSink.Connect(pAnim->AnimStop, &nuiNavigationController::OnViewPushStop, (void*)viewOverlay);
    pAnim->Play();
  }
  else 
  {
    AddChild(mpIn);  
  }
  
  
  if (!animated && mpOut && !viewOverlay)
  {
    mpOut->ViewDidDisappear();
    DelChild(mpOut);
  }

  if (!animated)
    if (mPendingOperations.size() >0)
      PopPendingOperation();
}




void nuiNavigationController::PopViewControllerAnimated(bool animated, TransitionType transition, bool viewOverlay)
{
  // store a pending operation and call for a asynchronous process
  mPendingOperations.push_back(PendingOperation(NULL, ePendingPop, animated, transition, viewOverlay));
  GetTopLevel()->PostNotification(new nuiNotification(NOTIF_PENDING_OPERATION));
}


void nuiNavigationController::_PopViewControllerAnimated(bool animated, TransitionType transition, bool viewOverlay)
{
  if (mViewControllers.size() < 1)
  {
    NGL_OUT(_T("nuiNavigationController::popViewControllerAnimated : nothing to pop"));
    return;
  }

  // don't overlapp animations
  if (mPushed || mPoped)
  {
    mPendingOperations.push_back(PendingOperation(NULL, ePendingPop, animated, transition, viewOverlay));
    return;
  }
  
  mpOut = mViewControllers[mViewControllers.size() -1];
  mpIn = NULL;
  if ((mViewControllers.size() > 1) && !viewOverlay)
    mpIn = mViewControllers[mViewControllers.size() -2];
  
  nuiRect idealsize = GetRect().Size();
  
  // animation was requested. launch animation and connect the event
  if (animated && (transition != eTransitionNone))
  {
    mPushed = false;
    mPoped = true;
    
    mAnimPosition = -idealsize.GetWidth();
    
    if (mpIn)
      AddChild(mpIn);  
    
    nuiAttributeAnimation* pAnim = new nuiAttributeAnimation();
    pAnim->SetTargetObject(this);
    pAnim->SetTargetAttribute(_T(".AnimPosition"));
    pAnim->SetStartValue(-idealsize.GetWidth());
    pAnim->SetEndValue(0);
    pAnim->SetEasing(mEasings[transition]);
    pAnim->SetDuration(mDurations[transition]);
    pAnim->SetDeleteOnStop(true);
    mEventSink.Connect(pAnim->AnimStop, &nuiNavigationController::OnViewPopStop, (void*)viewOverlay);
    pAnim->Play();
  }
  else
  {
    if (mpIn)
      AddChild(mpIn);  
    mpOut->ViewDidDisappear();
    mpOut->Release();
    DelChild(mpOut);
    mViewControllers.pop_back();

    if (mPendingOperations.size() >0)
      PopPendingOperation();
  }

}





// virtual 
bool nuiNavigationController::SetRect(const nuiRect& rRect)
{
  if (!mPushed && !mPoped)
    nuiSimpleContainer::SetRect(rRect);
  else
    nuiWidget::SetRect(rRect);
 
  // pending operation, if any...
  if (mPendingLayout)
  {
    mPendingLayout = false;
    if (mPendingOperations.size() >0)
      PopPendingOperation();
  }

  // nothing to layout right now...
  if (!mPushed && !mPoped)
    return true;

  
  nuiRect rect;
  rect.Set(mAnimPosition, rRect.Top(), rRect.GetWidth(), rRect.GetHeight());
  if (mpIn)
    mpIn->SetLayout(rect);
  
  if (mpOut)
  {
    if (mPushed)
      rect.Set(mAnimPosition - rRect.GetWidth(), rRect.Top(), rRect.GetWidth(), rRect.GetHeight());
    else
      rect.Set(mAnimPosition + rRect.GetWidth(), rRect.Top(), rRect.GetWidth(), rRect.GetHeight());
    
    mpOut->SetLayout(rect);
  }
  
  return true;
}


void nuiNavigationController::PopPendingOperation()
{
  if (mPendingOperations.size() == 0)
    return;

  PendingOperation op = mPendingOperations.front();
  mPendingOperations.pop_front();
 
  switch (op.mType)
  {
  case ePendingPush:
    _PushViewController(op.mpView, op.mAnimated, op.mTransition, op.mOverlay);
    break;
  case ePendingPop:
    _PopViewControllerAnimated(op.mAnimated, op.mTransition, op.mOverlay);
    break;
  case ePendingPopTo:
    _PopToViewController(op.mpView, op.mAnimated, op.mTransition);
    break;
  case ePendingPopToRoot:
    _PopToRootViewControllerAnimated(op.mAnimated, op.mTransition);
    break;
  }    

  UpdateLayout();
}



void nuiNavigationController::PopToViewController(nuiViewController* pViewController, bool animated, TransitionType transition)
{
  // store a pending operation and call for a asynchronous process
  mPendingOperations.push_back(PendingOperation(pViewController, ePendingPopTo, animated, transition, false));
  GetTopLevel()->PostNotification(new nuiNotification(NOTIF_PENDING_OPERATION));
}


void nuiNavigationController::_PopToViewController(nuiViewController* pViewController, bool animated, TransitionType transition)
{
 if (mViewControllers.size() < 1)
  {
    NGL_OUT(_T("nuiNavigationController::PopToViewController : nothing to pop"));
    return;
  }

  // don't overlapp animations
  if (mPushed || mPoped)
  {
    mPendingOperations.push_back(PendingOperation(pViewController, ePendingPopTo, animated, transition, false));
    return;
  }
  
}




void nuiNavigationController::PopToRootViewControllerAnimated(bool animated, TransitionType transition)
{
  // store a pending operation and call for a asynchronous process
  mPendingOperations.push_back(PendingOperation(NULL, ePendingPopToRoot, animated, transition, false));
  GetTopLevel()->PostNotification(new nuiNotification(NOTIF_PENDING_OPERATION));
}

void nuiNavigationController::_PopToRootViewControllerAnimated(bool animated, TransitionType transition)
{
 if (mViewControllers.size() < 1)
  {
    NGL_OUT(_T("nuiNavigationController::PopToRootViewControllerAnimated : nothing to pop"));
    return;
  }

  // don't overlapp animations
  if (mPushed || mPoped)
  {
    mPendingOperations.push_back(PendingOperation(NULL, ePendingPopToRoot, animated, transition, false));
    return;
  }
  
  
}



void nuiNavigationController::OnViewPushStop(const nuiEvent& rEvent)
{
  mPushed = false;
  mPoped = false;
  bool viewOverlay = (bool)rEvent.mpUser;
  
  if (mViewControllers.size() >1) 
  {
    if (!viewOverlay)
    {
      mpOut = mViewControllers[mViewControllers.size()-2];  
      mpOut->ViewDidDisappear();
      if (mpOut->GetParent())
        DelChild(mpOut);
    }

    if (mPendingOperations.size() >0)
      PopPendingOperation();
  }
  
}



void nuiNavigationController::OnViewPopStop(const nuiEvent& rEvent)
{
  mPushed = false;
  mPoped = false;
  bool viewOverlay = (bool)rEvent.mpUser;
  
  mpOut->ViewDidDisappear();
  if (mpOut->GetParent())
  {
    mpOut->Release();
    DelChild(mpOut);
    mViewControllers.pop_back(); 

    if (mPendingOperations.size() >0)
      PopPendingOperation();
  }
  
}




// virtual
void nuiNavigationController::ConnectTopLevel()
{
  GetTopLevel()->RegisterObserver(NOTIF_PENDING_OPERATION, this); 
}



//virtual 
void nuiNavigationController::OnNotification(const nuiNotification& rNotif)
{
  if (rNotif.GetName() == NOTIF_PENDING_OPERATION)
  {
    if (mPendingOperations.size() > 0)
      PopPendingOperation();
    return;
  }
}





