/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2009, 2010, 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "core/html/forms/InputType.h"

#include "InputTypeNames.h"
#include "RuntimeEnabledFeatures.h"
#include "core/accessibility/AXObjectCache.h"
#include "core/dom/NodeRenderStyle.h"
#include "core/events/KeyboardEvent.h"
#include "core/events/ScopedEventQueue.h"
#include "core/fileapi/FileList.h"
#include "core/html/FormDataList.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/forms/ButtonInputType.h"
#include "core/html/forms/CheckboxInputType.h"
#include "core/html/forms/ColorInputType.h"
#include "core/html/forms/DateInputType.h"
#include "core/html/forms/DateTimeLocalInputType.h"
#include "core/html/forms/EmailInputType.h"
#include "core/html/forms/FileInputType.h"
#include "core/html/forms/FormController.h"
#include "core/html/forms/HiddenInputType.h"
#include "core/html/forms/ImageInputType.h"
#include "core/html/forms/MonthInputType.h"
#include "core/html/forms/NumberInputType.h"
#include "core/html/forms/PasswordInputType.h"
#include "core/html/forms/RadioInputType.h"
#include "core/html/forms/RangeInputType.h"
#include "core/html/forms/ResetInputType.h"
#include "core/html/forms/SearchInputType.h"
#include "core/html/forms/SubmitInputType.h"
#include "core/html/forms/TelephoneInputType.h"
#include "core/html/forms/TextInputType.h"
#include "core/html/forms/TimeInputType.h"
#include "core/html/forms/URLInputType.h"
#include "core/html/forms/WeekInputType.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "core/html/shadow/HTMLShadowElement.h"
#include "core/page/Page.h"
#include "core/rendering/RenderTheme.h"
#include "platform/text/PlatformLocale.h"
#include "platform/text/TextBreakIterator.h"

namespace WebCore {

using blink::WebLocalizedString;
using namespace HTMLNames;
using namespace std;

typedef PassRefPtr<InputType> (*InputTypeFactoryFunction)(HTMLInputElement&);
typedef HashMap<AtomicString, InputTypeFactoryFunction, CaseFoldingHash> InputTypeFactoryMap;

static PassOwnPtr<InputTypeFactoryMap> createInputTypeFactoryMap()
{
    OwnPtr<InputTypeFactoryMap> map = adoptPtr(new InputTypeFactoryMap);
    map->add(InputTypeNames::button, ButtonInputType::create);
    map->add(InputTypeNames::checkbox, CheckboxInputType::create);
    if (RuntimeEnabledFeatures::inputTypeColorEnabled())
        map->add(InputTypeNames::color, ColorInputType::create);
    map->add(InputTypeNames::date, DateInputType::create);
    map->add(InputTypeNames::datetime_local, DateTimeLocalInputType::create);
    map->add(InputTypeNames::email, EmailInputType::create);
    map->add(InputTypeNames::file, FileInputType::create);
    map->add(InputTypeNames::hidden, HiddenInputType::create);
    map->add(InputTypeNames::image, ImageInputType::create);
    map->add(InputTypeNames::month, MonthInputType::create);
    map->add(InputTypeNames::number, NumberInputType::create);
    map->add(InputTypeNames::password, PasswordInputType::create);
    map->add(InputTypeNames::radio, RadioInputType::create);
    map->add(InputTypeNames::range, RangeInputType::create);
    map->add(InputTypeNames::reset, ResetInputType::create);
    map->add(InputTypeNames::search, SearchInputType::create);
    map->add(InputTypeNames::submit, SubmitInputType::create);
    map->add(InputTypeNames::tel, TelephoneInputType::create);
    map->add(InputTypeNames::time, TimeInputType::create);
    map->add(InputTypeNames::url, URLInputType::create);
    if (RuntimeEnabledFeatures::inputTypeWeekEnabled())
        map->add(InputTypeNames::week, WeekInputType::create);
    // No need to register "text" because it is the default type.
    return map.release();
}

static const InputTypeFactoryMap* factoryMap()
{
    static const InputTypeFactoryMap* factoryMap = createInputTypeFactoryMap().leakPtr();
    return factoryMap;
}

PassRefPtr<InputType> InputType::create(HTMLInputElement& element, const AtomicString& typeName)
{
    InputTypeFactoryFunction factory = typeName.isEmpty() ? 0 : factoryMap()->get(typeName);
    if (!factory)
        factory = TextInputType::create;
    return factory(element);
}

PassRefPtr<InputType> InputType::createText(HTMLInputElement& element)
{
    return TextInputType::create(element);
}

const AtomicString& InputType::normalizeTypeName(const AtomicString& typeName)
{
    if (typeName.isEmpty())
        return InputTypeNames::text;
    InputTypeFactoryMap::const_iterator it = factoryMap()->find(typeName);
    return it == factoryMap()->end() ? InputTypeNames::text : it->key;
}

bool InputType::canChangeFromAnotherType(const AtomicString& normalizedTypeName)
{
    // Don't allow the type to be changed to file after the first type change.
    // In other engines this might mean a JavaScript programmer could set a text
    // field's value to something like /etc/passwd and then change it to a file
    // input. I don't think this would actually occur in Blink, but this rule
    // still may be important for compatibility.
    return normalizedTypeName != InputTypeNames::file;
}

InputType::~InputType()
{
}

bool InputType::themeSupportsDataListUI(InputType* type)
{
    return RenderTheme::theme().supportsDataListUI(type->formControlType());
}

bool InputType::isTextField() const
{
    return false;
}

bool InputType::isTextType() const
{
    return false;
}

bool InputType::isRangeControl() const
{
    return false;
}

bool InputType::shouldSaveAndRestoreFormControlState() const
{
    return true;
}

FormControlState InputType::saveFormControlState() const
{
    String currentValue = element().value();
    if (currentValue == element().defaultValue())
        return FormControlState();
    return FormControlState(currentValue);
}

void InputType::restoreFormControlState(const FormControlState& state)
{
    element().setValue(state[0]);
}

bool InputType::isFormDataAppendable() const
{
    // There is no form data unless there's a name for non-image types.
    return !element().name().isEmpty();
}

bool InputType::appendFormData(FormDataList& encoding, bool) const
{
    // Always successful.
    encoding.appendData(element().name(), element().value());
    return true;
}

String InputType::resultForDialogSubmit() const
{
    return element().fastGetAttribute(valueAttr);
}

double InputType::valueAsDate() const
{
    return DateComponents::invalidMilliseconds();
}

void InputType::setValueAsDate(double, ExceptionState& exceptionState) const
{
    exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
}

double InputType::valueAsDouble() const
{
    return numeric_limits<double>::quiet_NaN();
}

void InputType::setValueAsDouble(double doubleValue, TextFieldEventBehavior eventBehavior, ExceptionState& exceptionState) const
{
    setValueAsDecimal(Decimal::fromDouble(doubleValue), eventBehavior, exceptionState);
}

void InputType::setValueAsDecimal(const Decimal&, TextFieldEventBehavior, ExceptionState& exceptionState) const
{
    exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
}

bool InputType::supportsValidation() const
{
    return true;
}

bool InputType::typeMismatchFor(const String&) const
{
    return false;
}

bool InputType::typeMismatch() const
{
    return false;
}

bool InputType::supportsRequired() const
{
    // Almost all validatable types support @required.
    return supportsValidation();
}

bool InputType::valueMissing(const String&) const
{
    return false;
}

bool InputType::hasBadInput() const
{
    return false;
}

bool InputType::patternMismatch(const String&) const
{
    return false;
}

bool InputType::rangeUnderflow(const String& value) const
{
    if (!isSteppable())
        return false;

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return false;

    return numericValue < createStepRange(RejectAny).minimum();
}

bool InputType::rangeOverflow(const String& value) const
{
    if (!isSteppable())
        return false;

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return false;

    return numericValue > createStepRange(RejectAny).maximum();
}

Decimal InputType::defaultValueForStepUp() const
{
    return 0;
}

double InputType::minimum() const
{
    return createStepRange(RejectAny).minimum().toDouble();
}

double InputType::maximum() const
{
    return createStepRange(RejectAny).maximum().toDouble();
}

bool InputType::isInRange(const String& value) const
{
    if (!isSteppable())
        return false;

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return true;

    StepRange stepRange(createStepRange(RejectAny));
    return numericValue >= stepRange.minimum() && numericValue <= stepRange.maximum();
}

bool InputType::isOutOfRange(const String& value) const
{
    if (!isSteppable())
        return false;

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return true;

    StepRange stepRange(createStepRange(RejectAny));
    return numericValue < stepRange.minimum() || numericValue > stepRange.maximum();
}

bool InputType::stepMismatch(const String& value) const
{
    if (!isSteppable())
        return false;

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return false;

    return createStepRange(RejectAny).stepMismatch(numericValue);
}

String InputType::badInputText() const
{
    ASSERT_NOT_REACHED();
    return locale().queryString(WebLocalizedString::ValidationTypeMismatch);
}

String InputType::rangeOverflowText(const Decimal&) const
{
    ASSERT_NOT_REACHED();
    return String();
}

String InputType::rangeUnderflowText(const Decimal&) const
{
    ASSERT_NOT_REACHED();
    return String();
}

String InputType::typeMismatchText() const
{
    return locale().queryString(WebLocalizedString::ValidationTypeMismatch);
}

String InputType::valueMissingText() const
{
    return locale().queryString(WebLocalizedString::ValidationValueMissing);
}

String InputType::validationMessage() const
{
    const String value = element().value();

    // The order of the following checks is meaningful. e.g. We'd like to show the
    // badInput message even if the control has other validation errors.
    if (hasBadInput())
        return badInputText();

    if (valueMissing(value))
        return valueMissingText();

    if (typeMismatch())
        return typeMismatchText();

    if (patternMismatch(value))
        return locale().queryString(WebLocalizedString::ValidationPatternMismatch);

    if (element().tooLong())
        return locale().validationMessageTooLongText(value.length(), element().maxLength());

    if (!isSteppable())
        return emptyString();

    const Decimal numericValue = parseToNumberOrNaN(value);
    if (!numericValue.isFinite())
        return emptyString();

    StepRange stepRange(createStepRange(RejectAny));

    if (numericValue < stepRange.minimum())
        return rangeUnderflowText(stepRange.minimum());

    if (numericValue > stepRange.maximum())
        return rangeOverflowText(stepRange.maximum());

    if (stepRange.stepMismatch(numericValue)) {
        ASSERT(stepRange.hasStep());
        Decimal candidate1 = stepRange.clampValue(numericValue);
        String localizedCandidate1 = localizeValue(serialize(candidate1));
        Decimal candidate2 = candidate1 < numericValue ? candidate1 + stepRange.step() : candidate1 - stepRange.step();
        if (!candidate2.isFinite() || candidate2 < stepRange.minimum() || candidate2 > stepRange.maximum())
            return locale().queryString(WebLocalizedString::ValidationStepMismatchCloseToLimit, localizedCandidate1);
        String localizedCandidate2 = localizeValue(serialize(candidate2));
        if (candidate1 < candidate2)
            return locale().queryString(WebLocalizedString::ValidationStepMismatch, localizedCandidate1, localizedCandidate2);
        return locale().queryString(WebLocalizedString::ValidationStepMismatch, localizedCandidate2, localizedCandidate1);
    }

    return emptyString();
}

bool InputType::shouldSubmitImplicitly(Event* event)
{
    return event->isKeyboardEvent() && event->type() == EventTypeNames::keypress && toKeyboardEvent(event)->charCode() == '\r';
}

Decimal InputType::parseToNumber(const String&, const Decimal& defaultValue) const
{
    ASSERT_NOT_REACHED();
    return defaultValue;
}

Decimal InputType::parseToNumberOrNaN(const String& string) const
{
    return parseToNumber(string, Decimal::nan());
}

bool InputType::parseToDateComponents(const String&, DateComponents*) const
{
    ASSERT_NOT_REACHED();
    return false;
}

String InputType::serialize(const Decimal&) const
{
    ASSERT_NOT_REACHED();
    return String();
}

void InputType::dispatchSimulatedClickIfActive(KeyboardEvent* event) const
{
    if (element().active())
        element().dispatchSimulatedClick(event);
    event->setDefaultHandled();
}

Chrome* InputType::chrome() const
{
    if (Page* page = element().document().page())
        return &page->chrome();
    return 0;
}

Locale& InputType::locale() const
{
    return element().locale();
}

bool InputType::canSetStringValue() const
{
    return true;
}

bool InputType::hasCustomFocusLogic() const
{
    return true;
}

bool InputType::isKeyboardFocusable() const
{
    return element().isFocusable();
}

bool InputType::shouldShowFocusRingOnMouseFocus() const
{
    return false;
}

bool InputType::shouldUseInputMethod() const
{
    return false;
}

void InputType::enableSecureTextInput()
{
}

void InputType::disableSecureTextInput()
{
}

void InputType::accessKeyAction(bool)
{
    element().focus(false);
}

void InputType::countUsage()
{
}

bool InputType::shouldRespectAlignAttribute()
{
    return false;
}

void InputType::sanitizeValueInResponseToMinOrMaxAttributeChange()
{
}

bool InputType::canBeSuccessfulSubmitButton()
{
    return false;
}

bool InputType::rendererIsNeeded()
{
    return true;
}

FileList* InputType::files()
{
    return 0;
}

void InputType::setFiles(PassRefPtr<FileList>)
{
}

bool InputType::getTypeSpecificValue(String&)
{
    return false;
}

String InputType::fallbackValue() const
{
    return String();
}

String InputType::defaultValue() const
{
    return String();
}

bool InputType::canSetSuggestedValue()
{
    return false;
}

bool InputType::shouldSendChangeEventAfterCheckedChanged()
{
    return true;
}

bool InputType::storesValueSeparateFromAttribute()
{
    return true;
}

void InputType::setValue(const String& sanitizedValue, bool valueChanged, TextFieldEventBehavior eventBehavior)
{
    element().setValueInternal(sanitizedValue, eventBehavior);
    element().setNeedsStyleRecalc();
    if (!valueChanged)
        return;
    switch (eventBehavior) {
    case DispatchChangeEvent:
        element().dispatchFormControlChangeEvent();
        break;
    case DispatchInputAndChangeEvent:
        element().dispatchFormControlInputEvent();
        element().dispatchFormControlChangeEvent();
        break;
    case DispatchNoEvent:
        break;
    }
}

bool InputType::canSetValue(const String&)
{
    return true;
}

String InputType::localizeValue(const String& proposedValue) const
{
    return proposedValue;
}

String InputType::visibleValue() const
{
    return element().value();
}

String InputType::sanitizeValue(const String& proposedValue) const
{
    return proposedValue;
}

bool InputType::receiveDroppedFiles(const DragData*)
{
    ASSERT_NOT_REACHED();
    return false;
}

String InputType::droppedFileSystemId()
{
    ASSERT_NOT_REACHED();
    return String();
}

bool InputType::shouldResetOnDocumentActivation()
{
    return false;
}

bool InputType::shouldRespectListAttribute()
{
    return false;
}

bool InputType::shouldRespectSpeechAttribute()
{
    return false;
}

bool InputType::isTextButton() const
{
    return false;
}

bool InputType::isRadioButton() const
{
    return false;
}

bool InputType::isSearchField() const
{
    return false;
}

bool InputType::isHiddenType() const
{
    return false;
}

bool InputType::isPasswordField() const
{
    return false;
}

bool InputType::isCheckbox() const
{
    return false;
}

bool InputType::isEmailField() const
{
    return false;
}

bool InputType::isFileUpload() const
{
    return false;
}

bool InputType::isImageButton() const
{
    return false;
}

bool InputType::isInteractiveContent() const
{
    return true;
}

bool InputType::isNumberField() const
{
    return false;
}

bool InputType::isSubmitButton() const
{
    return false;
}

bool InputType::isTelephoneField() const
{
    return false;
}

bool InputType::isURLField() const
{
    return false;
}

bool InputType::isDateField() const
{
    return false;
}

bool InputType::isDateTimeLocalField() const
{
    return false;
}

bool InputType::isMonthField() const
{
    return false;
}

bool InputType::isTimeField() const
{
    return false;
}

bool InputType::isWeekField() const
{
    return false;
}

bool InputType::isEnumeratable()
{
    return true;
}

bool InputType::isCheckable()
{
    return false;
}

bool InputType::isSteppable() const
{
    return false;
}

bool InputType::isColorControl() const
{
    return false;
}

bool InputType::shouldRespectHeightAndWidthAttributes()
{
    return false;
}

bool InputType::supportsPlaceholder() const
{
    return false;
}

bool InputType::supportsReadOnly() const
{
    return false;
}

void InputType::updatePlaceholderText()
{
}

String InputType::defaultToolTip() const
{
    return String();
}

Decimal InputType::findClosestTickMarkValue(const Decimal&)
{
    ASSERT_NOT_REACHED();
    return Decimal::nan();
}

void InputType::handleDOMActivateEvent(Event*)
{
}

bool InputType::supportsIndeterminateAppearance() const
{
    return false;
}

bool InputType::supportsInputModeAttribute() const
{
    return false;
}

bool InputType::supportsSelectionAPI() const
{
    return false;
}

unsigned InputType::height() const
{
    return 0;
}

unsigned InputType::width() const
{
    return 0;
}

void InputType::applyStep(int count, AnyStepHandling anyStepHandling, TextFieldEventBehavior eventBehavior, ExceptionState& exceptionState)
{
    StepRange stepRange(createStepRange(anyStepHandling));
    if (!stepRange.hasStep()) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }

    const Decimal current = parseToNumberOrNaN(element().value());
    if (!current.isFinite()) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }
    Decimal newValue = current + stepRange.step() * count;
    if (!newValue.isFinite()) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }

    const Decimal acceptableErrorValue = stepRange.acceptableError();
    if (newValue - stepRange.minimum() < -acceptableErrorValue) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }
    if (newValue < stepRange.minimum())
        newValue = stepRange.minimum();

    const AtomicString& stepString = element().fastGetAttribute(stepAttr);
    if (!equalIgnoringCase(stepString, "any"))
        newValue = stepRange.alignValueForStep(current, newValue);

    if (newValue - stepRange.maximum() > acceptableErrorValue) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }
    if (newValue > stepRange.maximum())
        newValue = stepRange.maximum();

    setValueAsDecimal(newValue, eventBehavior, exceptionState);

    if (AXObjectCache* cache = element().document().existingAXObjectCache())
        cache->postNotification(&element(), AXObjectCache::AXValueChanged, true);
}

bool InputType::getAllowedValueStep(Decimal* step) const
{
    StepRange stepRange(createStepRange(RejectAny));
    *step = stepRange.step();
    return stepRange.hasStep();
}

StepRange InputType::createStepRange(AnyStepHandling) const
{
    ASSERT_NOT_REACHED();
    return StepRange();
}

void InputType::stepUp(int n, ExceptionState& exceptionState)
{
    if (!isSteppable()) {
        exceptionState.throwUninformativeAndGenericDOMException(InvalidStateError);
        return;
    }
    applyStep(n, RejectAny, DispatchNoEvent, exceptionState);
}

void InputType::stepUpFromRenderer(int n)
{
    // The differences from stepUp()/stepDown():
    //
    // Difference 1: the current value
    // If the current value is not a number, including empty, the current value is assumed as 0.
    //   * If 0 is in-range, and matches to step value
    //     - The value should be the +step if n > 0
    //     - The value should be the -step if n < 0
    //     If -step or +step is out of range, new value should be 0.
    //   * If 0 is smaller than the minimum value
    //     - The value should be the minimum value for any n
    //   * If 0 is larger than the maximum value
    //     - The value should be the maximum value for any n
    //   * If 0 is in-range, but not matched to step value
    //     - The value should be the larger matched value nearest to 0 if n > 0
    //       e.g. <input type=number min=-100 step=3> -> 2
    //     - The value should be the smaler matched value nearest to 0 if n < 0
    //       e.g. <input type=number min=-100 step=3> -> -1
    //   As for date/datetime-local/month/time/week types, the current value is assumed as "the current local date/time".
    //   As for datetime type, the current value is assumed as "the current date/time in UTC".
    // If the current value is smaller than the minimum value:
    //  - The value should be the minimum value if n > 0
    //  - Nothing should happen if n < 0
    // If the current value is larger than the maximum value:
    //  - The value should be the maximum value if n < 0
    //  - Nothing should happen if n > 0
    //
    // Difference 2: clamping steps
    // If the current value is not matched to step value:
    // - The value should be the larger matched value nearest to 0 if n > 0
    //   e.g. <input type=number value=3 min=-100 step=3> -> 5
    // - The value should be the smaler matched value nearest to 0 if n < 0
    //   e.g. <input type=number value=3 min=-100 step=3> -> 2
    //
    // n is assumed as -n if step < 0.

    ASSERT(isSteppable());
    if (!isSteppable())
        return;
    ASSERT(n);
    if (!n)
        return;

    StepRange stepRange(createStepRange(AnyIsDefaultStep));

    // FIXME: Not any changes after stepping, even if it is an invalid value, may be better.
    // (e.g. Stepping-up for <input type="number" value="foo" step="any" /> => "foo")
    if (!stepRange.hasStep())
        return;

    EventQueueScope scope;
    const Decimal step = stepRange.step();

    int sign;
    if (step > 0)
        sign = n;
    else if (step < 0)
        sign = -n;
    else
        sign = 0;

    String currentStringValue = element().value();
    Decimal current = parseToNumberOrNaN(currentStringValue);
    if (!current.isFinite()) {
        current = defaultValueForStepUp();
        const Decimal nextDiff = step * n;
        if (current < stepRange.minimum() - nextDiff)
            current = stepRange.minimum() - nextDiff;
        if (current > stepRange.maximum() - nextDiff)
            current = stepRange.maximum() - nextDiff;
        setValueAsDecimal(current, DispatchNoEvent, IGNORE_EXCEPTION);
    }
    if ((sign > 0 && current < stepRange.minimum()) || (sign < 0 && current > stepRange.maximum())) {
        setValueAsDecimal(sign > 0 ? stepRange.minimum() : stepRange.maximum(), DispatchInputAndChangeEvent, IGNORE_EXCEPTION);
    } else {
        if (stepMismatch(element().value())) {
            ASSERT(!step.isZero());
            const Decimal base = stepRange.stepBase();
            Decimal newValue;
            if (sign < 0)
                newValue = base + ((current - base) / step).floor() * step;
            else if (sign > 0)
                newValue = base + ((current - base) / step).ceiling() * step;
            else
                newValue = current;

            if (newValue < stepRange.minimum())
                newValue = stepRange.minimum();
            if (newValue > stepRange.maximum())
                newValue = stepRange.maximum();

            setValueAsDecimal(newValue, n == 1 || n == -1 ? DispatchInputAndChangeEvent : DispatchNoEvent, IGNORE_EXCEPTION);
            if (n > 1)
                applyStep(n - 1, AnyIsDefaultStep, DispatchInputAndChangeEvent, IGNORE_EXCEPTION);
            else if (n < -1)
                applyStep(n + 1, AnyIsDefaultStep, DispatchInputAndChangeEvent, IGNORE_EXCEPTION);
        } else {
            applyStep(n, AnyIsDefaultStep, DispatchInputAndChangeEvent, IGNORE_EXCEPTION);
        }
    }
}

void InputType::countUsageIfVisible(UseCounter::Feature feature) const
{
    if (RenderStyle* style = element().renderStyle()) {
        if (style->visibility() != HIDDEN)
            UseCounter::count(element().document(), feature);
    }
}

Decimal InputType::findStepBase(const Decimal& defaultValue) const
{
    Decimal stepBase = parseToNumber(element().fastGetAttribute(minAttr), Decimal::nan());
    if (!stepBase.isFinite())
        stepBase = parseToNumber(element().fastGetAttribute(valueAttr), defaultValue);
    return stepBase;
}

StepRange InputType::createStepRange(AnyStepHandling anyStepHandling, const Decimal& stepBaseDefault, const Decimal& minimumDefault, const Decimal& maximumDefault, const StepRange::StepDescription& stepDescription) const
{
    const Decimal stepBase = findStepBase(stepBaseDefault);
    const Decimal minimum = parseToNumber(element().fastGetAttribute(minAttr), minimumDefault);
    const Decimal maximum = parseToNumber(element().fastGetAttribute(maxAttr), maximumDefault);
    const Decimal step = StepRange::parseStep(anyStepHandling, stepDescription, element().fastGetAttribute(stepAttr));
    return StepRange(stepBase, minimum, maximum, step, stepDescription);
}

} // namespace WebCore
