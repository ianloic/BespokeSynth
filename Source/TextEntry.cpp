/**
    bespoke synth, a software modular synthesizer
    Copyright (C) 2021 Ryan Challinor (contact: awwbees@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

//
//  TextEntry.cpp
//  modularSynth
//
//  Created by Ryan Challinor on 12/5/12.
//
//

#include "TextEntry.h"
#include "SynthGlobals.h"
#include "IDrawableModule.h"
#include "FileStream.h"

IKeyboardFocusListener* IKeyboardFocusListener::sCurrentKeyboardFocus = nullptr;

//static
void IKeyboardFocusListener::ClearActiveKeyboardFocus(bool notifyListeners)
{
   if (sCurrentKeyboardFocus && notifyListeners)
      sCurrentKeyboardFocus->AcceptEntry(false);
   sCurrentKeyboardFocus = nullptr;
}

TextEntry::TextEntry(ITextEntryListener* owner, const char* name, int x, int y, int charWidth, char* var)
: mVarCString(var)
, mVarString(nullptr)
, mVarInt(nullptr)
, mVarFloat(nullptr)
, mType(kTextEntry_Text)
{
   Construct(owner, name, x, y, charWidth);
}

TextEntry::TextEntry(ITextEntryListener* owner, const char* name, int x, int y, int charWidth, string* var)
: mVarCString(nullptr)
, mVarString(var)
, mVarInt(nullptr)
, mVarFloat(nullptr)
, mType(kTextEntry_Text)
{
   Construct(owner, name, x, y, charWidth);
}

TextEntry::TextEntry(ITextEntryListener* owner, const char* name, int x, int y, int charWidth, int* var, int min, int max)
: mVarCString(nullptr)
, mVarString(nullptr)
, mVarInt(var)
, mVarFloat(nullptr)
, mType(kTextEntry_Int)
, mIntMin(min)
, mIntMax(max)
{
   Construct(owner, name, x, y, charWidth);
}

TextEntry::TextEntry(ITextEntryListener* owner, const char* name, int x, int y, int charWidth, float* var, float min, float max)
: mVarCString(nullptr)
, mVarString(nullptr)
, mVarInt(nullptr)
, mVarFloat(var)
, mType(kTextEntry_Float)
, mFloatMin(min)
, mFloatMax(max)
{
   Construct(owner, name, x, y, charWidth);
}

void TextEntry::Construct(ITextEntryListener* owner, const char* name, int x, int y, int charWidth)
{
   mCharWidth = charWidth;
   mListener = owner;
   mCaretBlink = true;
   mCaretBlinkTimer = 0;
   mNextTextEntry = nullptr;
   mPreviousTextEntry = nullptr;
   mInErrorMode = false;
   mDrawLabel = false;
   mFlexibleWidth = false;
   mHovered = false;
   mRequireEnterToAccept = false;
   
   UpdateDisplayString();
   
   SetName(name);
   mLabelSize = gFontFixedWidth.GetStringWidth(name, 14) + 3 + .25f * strnlen(name, 50);
   SetPosition(x,y);
   assert(owner);
   IDrawableModule* module = dynamic_cast<IDrawableModule*>(owner);
   if (module)
      module->AddUIControl(this);
   SetParent(dynamic_cast<IClickable*>(owner));
}

TextEntry::~TextEntry()
{
}

void TextEntry::Delete()
{
   if (IKeyboardFocusListener::GetActiveKeyboardFocus() == this)
      IKeyboardFocusListener::ClearActiveKeyboardFocus(false);
   delete this;
}

void TextEntry::Render()
{
   ofPushStyle();
   
   ofSetLineWidth(.5f);
   
   float xOffset = 0;
   if (mDrawLabel)
   {
      DrawTextNormal(Name(), mX, mY+12);
      xOffset = mLabelSize;
   }
   
   bool isCurrent = IKeyboardFocusListener::GetActiveKeyboardFocus() == this;
   
   ofColor color(255,255,255);
   if (!isCurrent && mInErrorMode)
      color.set(200,100,100);

   if (!isCurrent)
      UpdateDisplayString();
   
   float w,h;
   GetDimensions(w,h);
   if (isCurrent)
   {
      ofSetColor(color,gModuleDrawAlpha * .1f);
      ofFill();
      ofRect(mX + xOffset,mY,w - xOffset,h);
   }
   ofSetColor(color,gModuleDrawAlpha);
   ofNoFill();
   ofRect(mX + xOffset,mY,w - xOffset,h);
   gFontFixedWidth.DrawString(mString, 14, mX+2+xOffset, mY+12);
   
   if (IKeyboardFocusListener::GetActiveKeyboardFocus() == this)
   {
      if (mCaretBlink)
      {
         int caretX = mX+2+xOffset;
         int caretY = mY+1;
         if (mCaretPosition > 0)
         {
            char beforeCaret[MAX_TEXTENTRY_LENGTH];
            strncpy(beforeCaret, mString, mCaretPosition);
            beforeCaret[mCaretPosition] = 0;
            caretX += gFontFixedWidth.GetStringWidth(beforeCaret, 14);
         }
         ofFill();
         ofRect(caretX, caretY, 1, 12, L(corner, 1));
      }
      mCaretBlinkTimer += ofGetLastFrameTime();
      if (mCaretBlinkTimer > .3f)
      {
         mCaretBlinkTimer -= .3f;
         mCaretBlink = !mCaretBlink;
      }
   }
   
   /*if (mHovered)
   {
      ofSetColor(100, 100, 100, .8f*gModuleDrawAlpha);
      ofFill();
      ofRect(mX+xOffset,mY-12,GetStringWidth(Name()),12);
      ofSetColor(255, 255, 255, gModuleDrawAlpha);
      DrawTextNormal(Name(), mX+xOffset, mY);
   }*/

   ofPopStyle();
   
   DrawHover(mX + xOffset, mY, w - xOffset, h);
}

void TextEntry::GetDimensions(float& width, float& height)
{
   if (mFlexibleWidth)
      width = MAX(30.0f,gFontFixedWidth.GetStringWidth(mString, 14) + 4);
   else
      width = mCharWidth * 9;
   
   if (mDrawLabel)
      width += mLabelSize;
   
   height = 15;
}

void TextEntry::OnClicked(int x, int y, bool right)
{
   if (right)
      return;
   
   float xOffset = 2;
   if (mDrawLabel)
      xOffset += mLabelSize;
   
   mCaretPosition = 0;
   
   char caretCheck[MAX_TEXTENTRY_LENGTH];
   size_t checkLength = strnlen(mString, MAX_TEXTENTRY_LENGTH);
   strncpy(caretCheck, mString, checkLength);
   int lastSubstrWidth = gFontFixedWidth.GetStringWidth(caretCheck, 14);
   for (int i=(int)checkLength-1; i >= 0; --i)
   {
      caretCheck[i] = 0;   //shorten string by one
      
      int substrWidth = gFontFixedWidth.GetStringWidth(caretCheck, 14);
      //ofLog() << x << " " << i << " " << (xOffset + substrWidth);
      if (x > xOffset + ((substrWidth + lastSubstrWidth) * .5f))
      {
         mCaretPosition = i + 1;
         break;
      }
      
      lastSubstrWidth = substrWidth;
   }
   
   MakeActiveTextEntry(false);
}

void TextEntry::MakeActiveTextEntry(bool setCaretToEnd)
{
   SetActiveKeyboardFocus(this);
   if (mListener)
      mListener->TextEntryActivated(this);
   if (setCaretToEnd)
      mCaretPosition = (int)strlen(mString);
   mCaretBlink = true;
   mCaretBlinkTimer = 0;
}

void TextEntry::OnKeyPressed(int key, bool isRepeat)
{
   if (key == OF_KEY_RETURN)
   {
      AcceptEntry(true);
      IKeyboardFocusListener::ClearActiveKeyboardFocus(!K(notifyListeners));
   }
   if (key == OF_KEY_TAB)
   {
      TextEntry* pendingNewEntry = nullptr;
      if (GetKeyModifiers() == kModifier_Shift)
         pendingNewEntry = mPreviousTextEntry;
      else
         pendingNewEntry = mNextTextEntry;

      AcceptEntry(false);
      IKeyboardFocusListener::ClearActiveKeyboardFocus(!K(notifyListeners));
      
      if (pendingNewEntry)
         pendingNewEntry->MakeActiveTextEntry(true);
   }
   else if (key == OF_KEY_BACKSPACE)
   {
      int len = (int)strlen(mString);
      if (mCaretPosition > 0)
      {
         for (int i=mCaretPosition-1; i<len; ++i)
            mString[i] = mString[i + 1];
         --mCaretPosition;
      }
   }
   else if (key == KeyPress::deleteKey)
   {
      int len = (int)strlen(mString);
      for (int i = mCaretPosition; i < len; ++i)
         mString[i] = mString[i + 1];
   }
   else if (key == OF_KEY_ESC)
   {
      IKeyboardFocusListener::ClearActiveKeyboardFocus(K(notifyListeners));
   }
   else if (key == OF_KEY_LEFT)
   {
      if (GetKeyModifiers() & kModifier_Command)
         mCaretPosition = 0;
      else if (mCaretPosition > 0)
         --mCaretPosition;
   }
   else if (key == OF_KEY_RIGHT)
   {
      if (GetKeyModifiers() & kModifier_Command)
         mCaretPosition = (int)strlen(mString);
      else if (mCaretPosition < (int)strlen(mString))
         ++mCaretPosition;
   }
   else if (key == OF_KEY_UP)
   {
      if (mType == kTextEntry_Float)
      {
         if (*mVarFloat + 1 <= mFloatMax)
         {
            *mVarFloat += 1;
            UpdateDisplayString();
            AcceptEntry(false);
         }
      }
      if (mType == kTextEntry_Int)
      {
         if (*mVarInt + 1 <= mIntMax)
         {
            *mVarInt += 1;
            UpdateDisplayString();
            AcceptEntry(false);
         }
      }
   }
   else if (key == OF_KEY_DOWN)
   {
      if (mType == kTextEntry_Float)
      {
         if (*mVarFloat - 1 >= mFloatMin)
         {
            *mVarFloat -= 1;
            UpdateDisplayString();
            AcceptEntry(false);
         }
      }
      if (mType == kTextEntry_Int)
      {
         if (*mVarInt - 1 >= mIntMin)
         {
            *mVarInt -= 1;
            UpdateDisplayString();
            AcceptEntry(false);
         }
      }
   }
   else if (toupper(key) == 'V' && GetKeyModifiers() == kModifier_Command)
   {
      juce::String clipboard = SystemClipboard::getTextFromClipboard();
      for (int i=0; i<clipboard.length(); ++i)
         AddCharacter(clipboard[i]);
   }
   else if (key == KeyPress::homeKey)
   {
      mCaretPosition = 0;
   }
   else if (key == KeyPress::endKey)
   {
      mCaretPosition = (int)strlen(mString);
   }
   else if (key < CHAR_MAX && juce::CharacterFunctions::isPrintable((char)key))
   {
      AddCharacter((char)key);
   }
}

void TextEntry::AddCharacter(char c)
{
   if (AllowCharacter(c))
   {
      int len = (int)strlen(mString);
      if (/*len < mCharWidth && */len < MAX_TEXTENTRY_LENGTH-1)
      {
         for (int i=len; i>mCaretPosition; --i)
            mString[i] = mString[i-1];
         mString[mCaretPosition] = c;
         ++mCaretPosition;
      }
   }
}

void TextEntry::UpdateDisplayString()
{
   if (mVarCString)
      StringCopy(mString, mVarCString, MAX_TEXTENTRY_LENGTH);
   if (mVarString)
      StringCopy(mString, mVarString->c_str(), MAX_TEXTENTRY_LENGTH);
   if (mVarInt)
      StringCopy(mString, ofToString(*mVarInt).c_str(), MAX_TEXTENTRY_LENGTH);
   if (mVarFloat)
      StringCopy(mString, ofToString(*mVarFloat).c_str(), MAX_TEXTENTRY_LENGTH);
}

void TextEntry::SetValue(float value)
{
   if (mType == kTextEntry_Int)
   {
      *mVarInt = value;
      UpdateDisplayString();
   }

   if (mType == kTextEntry_Float)
   {
      *mVarFloat = value;
      UpdateDisplayString();
   }
}

void TextEntry::AcceptEntry(bool pressedEnter)
{
   if (!pressedEnter && mRequireEnterToAccept)
   {
      CancelEntry();
      return;
   }
   
   if (mVarCString)
      StringCopy(mVarCString, mString, MAX_TEXTENTRY_LENGTH);
   if (mVarString)
      *mVarString = mString;
   if (mVarInt && mString[0] != 0)
   {
      *mVarInt = ofClamp(ofToInt(mString), mIntMin, mIntMax);
      StringCopy(mString, ofToString(*mVarInt).c_str(), MAX_TEXTENTRY_LENGTH);
   }
   if (mVarFloat && mString[0] != 0)
   {
      *mVarFloat = ofClamp(ofToFloat(mString), mFloatMin, mFloatMax);
      StringCopy(mString, ofToString(*mVarFloat).c_str(), MAX_TEXTENTRY_LENGTH);
   }
   
   if (mListener)
      mListener->TextEntryComplete(this);
}

void TextEntry::CancelEntry()
{
   if (mListener)
      mListener->TextEntryCancelled(this);
}

bool TextEntry::AllowCharacter(char c)
{
   if (mType == kTextEntry_Text)
      return juce::CharacterFunctions::isPrintable(c);
   if (mType == kTextEntry_Int)
      return CharacterFunctions::isDigit((char)c) || c == '-';
   if (mType == kTextEntry_Float)
      return CharacterFunctions::isDigit((char)c) || c == '.' || c == '-';
   return false;
}

void TextEntry::Increment(float amount)
{
   if (mType == kTextEntry_Float)
   {
      float newVal = *mVarFloat + amount;
      if (newVal >= mFloatMin && newVal <= mFloatMax)
      {
         *mVarFloat = newVal;
         UpdateDisplayString();
         AcceptEntry(false);
      }
   }
   if (mType == kTextEntry_Int)
   {
      int newVal = *mVarInt + (int)amount;
      if (newVal >= mIntMin && newVal <= mIntMax)
      {
         *mVarInt = newVal;
         UpdateDisplayString();
         AcceptEntry(false);
      }
   }
}

void TextEntry::SetNextTextEntry(TextEntry* entry)
{
   mNextTextEntry = entry;
   if (entry)
      entry->mPreviousTextEntry = this;
}

bool TextEntry::MouseMoved(float x, float y)
{
   mHovered = TestHover(x, y);
   CheckHover(x, y);
   return false;
}

namespace
{
   const int kSaveStateRev = 0;
}

void TextEntry::SaveState(FileStreamOut& out)
{
   out << kSaveStateRev;
   
   out << string(mString);
}

void TextEntry::LoadState(FileStreamIn& in, bool shouldSetValue)
{
   int rev;
   in >> rev;
   LoadStateValidate(rev == kSaveStateRev);
   
   string var;
   in >> var;
   StringCopy(mString, var.c_str(), MAX_TEXTENTRY_LENGTH);
   AcceptEntry(false);
}
