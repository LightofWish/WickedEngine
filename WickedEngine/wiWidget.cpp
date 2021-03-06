#include "wiWidget.h"
#include "wiGUI.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiFont.h"
#include "wiMath.h"
#include "wiHelper.h"
#include "wiInputManager.h"

#include <DirectXCollision.h>

using namespace std;
using namespace wiGraphicsTypes;


wiWidget::wiWidget():Transform()
{
	state = IDLE;
	enabled = true;
	visible = true;
	colors[IDLE] = wiColor::Booger;
	colors[FOCUS] = wiColor::Gray;
	colors[ACTIVE] = wiColor::White;
	colors[DEACTIVATING] = wiColor::Gray;
	scissorRect.bottom = 0;
	scissorRect.left = 0;
	scissorRect.right = 0;
	scissorRect.top = 0;
	container = nullptr;
	tooltipTimer = 0;
	textColor = wiColor(255, 255, 255, 255);
	textShadowColor = wiColor(0, 0, 0, 255);
}
wiWidget::~wiWidget()
{
}
void wiWidget::Update(wiGUI* gui, float dt )
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (GetState() == WIDGETSTATE::FOCUS && !tooltip.empty())
	{
		tooltipTimer++;
	}
	else
	{
		tooltipTimer = 0;
	}

	// Only do the updatetransform if it has no parent because if it has, its transform
	// will be updated down the chain anyway
	if (Transform::parent == nullptr)
	{
		Transform::UpdateTransform();
	}
}
void wiWidget::RenderTooltip(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (tooltipTimer > 25)
	{
		XMFLOAT2 tooltipPos = XMFLOAT2(gui->pointerpos.x, gui->pointerpos.y);
		if (tooltipPos.y > wiRenderer::GetDevice()->GetScreenHeight()*0.8f)
		{
			tooltipPos.y -= 30;
		}
		else
		{
			tooltipPos.y += 40;
		}
		wiFontProps fontProps = wiFontProps((int)tooltipPos.x, (int)tooltipPos.y, -1, WIFALIGN_LEFT, WIFALIGN_TOP);
		fontProps.color = wiColor(25, 25, 25, 255);
		wiFont tooltipFont = wiFont(tooltip, fontProps);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(tooltip + "\n" + scriptTip);
		}
		static const float _border = 2;
		float fontWidth = (float)tooltipFont.textWidth() + _border* 2;
		float fontHeight = (float)tooltipFont.textHeight() + _border * 2;
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor(255, 234, 165)), wiImageEffects(tooltipPos.x - _border, tooltipPos.y - _border, fontWidth, fontHeight), gui->GetGraphicsThread());
		tooltipFont.SetText(tooltip);
		tooltipFont.Draw(gui->GetGraphicsThread());
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(scriptTip);
			tooltipFont.props.posY += (int)(fontHeight / 2);
			tooltipFont.props.color = wiColor(25, 25, 25, 110);
			tooltipFont.Draw(gui->GetGraphicsThread());
		}
	}
}
wiHashString wiWidget::GetName()
{
	return fastName;
}
void wiWidget::SetName(const std::string& value)
{
	name = value;
	if (value.length() <= 0)
	{
		static unsigned long widgetID = 0;
		stringstream ss("");
		ss << "widget_" << widgetID;
		name = ss.str();
		widgetID++;
	}

	fastName = wiHashString(name);
}
string wiWidget::GetText()
{
	return name;
}
void wiWidget::SetText(const std::string& value)
{
	text = value;
}
void wiWidget::SetTooltip(const std::string& value)
{
	tooltip = value;
}
void wiWidget::SetScriptTip(const std::string& value)
{
	scriptTip = value;
}
void wiWidget::SetPos(const XMFLOAT2& value)
{
	Transform::translation_rest.x = value.x;
	Transform::translation_rest.y = value.y;
	Transform::UpdateTransform();
}
void wiWidget::SetSize(const XMFLOAT2& value)
{
	Transform::scale_rest.x = value.x;
	Transform::scale_rest.y = value.y;
	Transform::UpdateTransform();
}
wiWidget::WIDGETSTATE wiWidget::GetState()
{
	return state;
}
void wiWidget::SetEnabled(bool val) 
{ 
	enabled = val; 
}
bool wiWidget::IsEnabled() 
{ 
	return enabled && visible; 
}
void wiWidget::SetVisible(bool val)
{ 
	visible = val;
}
bool wiWidget::IsVisible() 
{ 
	return visible;
}
void wiWidget::Activate()
{
	state = ACTIVE;
}
void wiWidget::Deactivate()
{
	state = DEACTIVATING;
}
void wiWidget::SetColor(const wiColor& color, WIDGETSTATE state)
{
	if (state == WIDGETSTATE_COUNT)
	{
		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			colors[i] = color;
		}
	}
	else
	{
		colors[state] = color;
	}
}
wiColor wiWidget::GetColor()
{
	wiColor retVal = colors[GetState()];
	if (!IsEnabled()) {
		retVal = wiColor::lerp(wiColor::Transparent, retVal, 0.5f);
	}
	return retVal;
}
void wiWidget::SetScissorRect(const wiGraphicsTypes::Rect& rect)
{
	scissorRect = rect;
	if(scissorRect.bottom>0)
		scissorRect.bottom -= 1;
	if (scissorRect.left>0)
		scissorRect.left += 1;
	if (scissorRect.right>0)
		scissorRect.right -= 1;
	if (scissorRect.top>0)
		scissorRect.top += 1;
}


wiButton::wiButton(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		onDragEnd(args);

		if (pointerHitbox.intersects(hitBox))
		{
			// Click occurs when the button is released within the bounds
			onClick(args);
		}

		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);

			wiEventArgs args;
			args.clickPos = pointerHitbox.pos;
			XMFLOAT3 posDelta;
			posDelta.x = pointerHitbox.pos.x - prevPos.x;
			posDelta.y = pointerHitbox.pos.y - prevPos.y;
			posDelta.z = 0;
			args.deltaPos = XMFLOAT2(posDelta.x, posDelta.y);
			onDrag(args);
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		dragStart = args.clickPos;
		args.startPos = dragStart;
		onDragStart(args);
		gui->ActivateWidget(this);
	}

	prevPos.x = pointerHitbox.pos.x;
	prevPos.y = pointerHitbox.pos.y;

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());



	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)(translation.x + scale.x*0.5f), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1, 
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), true);

}
void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiButton::OnDragStart(function<void(wiEventArgs args)> func)
{
	onDragStart = move(func);
}
void wiButton::OnDrag(function<void(wiEventArgs args)> func)
{
	onDrag = move(func);
}
void wiButton::OnDragEnd(function<void(wiEventArgs args)> func)
{
	onDragEnd = move(func);
}




wiLabel::wiLabel(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(100, 20));
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiLabel::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());


	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)translation.x, (int)translation.y, -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1, 
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), true);

}




wiSlider::wiSlider(float start, float end, float defaultValue, float step, const std::string& name) :wiWidget()
	,start(start), end(end), value(defaultValue), step(max(step, 1))
{
	SetName(name);
	SetText(fastName.GetString());
	OnSlide([](wiEventArgs args) {});
	SetSize(XMFLOAT2(200, 40));
}
wiSlider::~wiSlider()
{
}
void wiSlider::SetValue(float value)
{
	this->value = value;
}
float wiSlider::GetValue()
{
	return value;
}
void wiSlider::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	bool dragged = false;

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			if (state == ACTIVE)
			{
				// continue drag if already grabbed wheter it is intersecting or not
				dragged = true;
			}
		}
		else
		{
			gui->DeactivateWidget(this);
		}
	}

	float headWidth = scale.x*0.05f;

	hitBox.pos.x = Transform::translation.x - headWidth*0.5f;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x + headWidth;
	hitBox.siz.y = Transform::scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));


	if (pointerHitbox.intersects(hitBox))
	{
		// hover the slider
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}


	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		value = wiMath::InverseLerp(translation.x, translation.x + scale.x, args.clickPos.x);
		value = wiMath::Clamp(value, 0, 1);
		value *= step;
		value = floorf(value);
		value /= step;
		value = wiMath::Lerp(start, end, value);
		args.fValue = value;
		args.iValue = (int)value;
		onSlide(args);
		gui->ActivateWidget(this);
	}

}
void wiSlider::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	float headWidth = scale.x*0.05f;

	// trail
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x - headWidth*0.5f, translation.y + scale.y * 0.5f - scale.y*0.1f, scale.x + headWidth, scale.y * 0.2f), gui->GetGraphicsThread());
	// head
	float headPosX = wiMath::Lerp(translation.x, translation.x + scale.x, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(headPosX - headWidth * 0.5f, translation.y, headWidth, scale.y), gui->GetGraphicsThread());

	if (parent != nullptr)
	{
		wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	}
	// text
	wiFont(text, wiFontProps((int)(translation.x - headWidth * 0.5f), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor )).Draw(gui->GetGraphicsThread(), parent != nullptr);
	// value
	stringstream ss("");
	ss << value;
	wiFont(ss.str(), wiFontProps((int)(translation.x + scale.x + headWidth), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_LEFT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor )).Draw(gui->GetGraphicsThread(), parent != nullptr);


}
void wiSlider::OnSlide(function<void(wiEventArgs args)> func)
{
	onSlide = move(func);
}





wiCheckBox::wiCheckBox(const std::string& name) :wiWidget()
	,checked(false)
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(20,20));
}
wiCheckBox::~wiCheckBox()
{

}
void wiCheckBox::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x;
	hitBox.siz.y = Transform::scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		SetCheck(!GetCheck());
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		args.bValue = GetCheck();
		onClick(args);
		gui->ActivateWidget(this);
	}

}
void wiCheckBox::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	// control
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());

	// check
	if (GetCheck())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor::lerp(color, wiColor::White, 0.8f))
			, wiImageEffects(translation.x + scale.x*0.25f, translation.y + scale.y*0.25f, scale.x*0.5f, scale.y*0.5f)
			, gui->GetGraphicsThread());
	}

	if (parent != nullptr)
	{
		wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	}
	wiFont(text, wiFontProps((int)(translation.x), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor )).Draw(gui->GetGraphicsThread(), parent != nullptr);

}
void wiCheckBox::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiCheckBox::SetCheck(bool value)
{
	checked = value;
}
bool wiCheckBox::GetCheck()
{
	return checked;
}





wiComboBox::wiComboBox(const std::string& name) :wiWidget()
, selected(-1), combostate(COMBOSTATE_INACTIVE), hovered(-1)
{
	SetName(name);
	SetText(fastName.GetString());
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));
	SetMaxVisibleItemCount(8);
	firstItemVisible = 0;
}
wiComboBox::~wiComboBox()
{

}
const float wiComboBox::_GetItemOffset(int index) const
{
	index = max(firstItemVisible, index) - firstItemVisible;
	return scale.y * (index + 1) + 1;
}
void wiComboBox::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE && combostate == COMBOSTATE_SELECTING)
	{
		gui->DeactivateWidget(this);
	}

	hitBox.pos.x = Transform::translation.x;
	hitBox.pos.y = Transform::translation.y;
	hitBox.siz.x = Transform::scale.x + scale.y + 1; // + drop-down indicator arrow + little offset
	hitBox.siz.y = Transform::scale.y;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		// activate
		clicked = true;
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}


	if (clicked && state == FOCUS)
	{
		gui->ActivateWidget(this);
	}


	if (state == ACTIVE)
	{
		if (combostate == COMBOSTATE_INACTIVE)
		{
			combostate = COMBOSTATE_HOVER;
		}
		else if (combostate == COMBOSTATE_SELECTING)
		{
			gui->DeactivateWidget(this);
			combostate = COMBOSTATE_INACTIVE;
		}
		else
		{
			int scroll = (int)wiInputManager::GetInstance()->getpointer().z;
			firstItemVisible -= scroll;
			firstItemVisible = max(0, min((int)items.size() - maxVisibleItemCount, firstItemVisible));

			hovered = -1;
			for (size_t i = 0; i < items.size(); ++i)
			{
				if ((int)i<firstItemVisible || (int)i>firstItemVisible + maxVisibleItemCount)
				{
					continue;
				}

				Hitbox2D itembox;
				itembox.pos.x = Transform::translation.x;
				itembox.pos.y = Transform::translation.y + _GetItemOffset((int)i);
				itembox.siz.x = Transform::scale.x;
				itembox.siz.y = Transform::scale.y;
				if (pointerHitbox.intersects(itembox))
				{
					hovered = (int)i;
					break;
				}
			}

			if (clicked)
			{
				combostate = COMBOSTATE_SELECTING;
				if (hovered >= 0)
				{
					SetSelected(hovered);
				}
			}
		}
	}

}
void wiComboBox::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (combostate != COMBOSTATE_INACTIVE)
	{
		color = colors[FOCUS];
	}

	// control-base
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());
	// control-arrow
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(translation.x+scale.x+1, translation.y, scale.y, scale.y), gui->GetGraphicsThread());
	wiFont("V", wiFontProps((int)(translation.x+scale.x+scale.y*0.5f), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), false);


	if (parent != nullptr)
	{
		wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	}
	wiFont(text, wiFontProps((int)(translation.x), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), parent != nullptr);

	if (selected >= 0)
	{
		wiFont(items[selected], wiFontProps((int)(translation.x + scale.x*0.5f), (int)(translation.y + scale.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
			textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), parent != nullptr);
	}

	// drop-down
	if (state == ACTIVE)
	{
		// control-list
		int i = 0;
		for (auto& x : items)
		{
			if (i<firstItemVisible || i>firstItemVisible + maxVisibleItemCount)
			{
				i++;
				continue;
			}

			wiColor col = colors[IDLE];
			if (hovered == i)
			{
				if (combostate == COMBOSTATE_HOVER)
				{
					col = colors[FOCUS];
				}
				else if (combostate == COMBOSTATE_SELECTING)
				{
					col = colors[ACTIVE];
				}
			}
			wiImage::Draw(wiTextureHelper::getInstance()->getColor(col)
				, wiImageEffects(translation.x, translation.y + _GetItemOffset(i), scale.x, scale.y), gui->GetGraphicsThread());
			wiFont(x, wiFontProps((int)(translation.x + scale.x*0.5f), (int)(translation.y + scale.y*0.5f +_GetItemOffset(i)), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
				textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), false);
			i++;
		}
	}
}
void wiComboBox::OnSelect(function<void(wiEventArgs args)> func)
{
	onSelect = move(func);
}
void wiComboBox::AddItem(const std::string& item)
{
	items.push_back(item);

	if (selected < 0)
	{
		selected = 0;
	}
}
void wiComboBox::RemoveItem(int index)
{
	std::vector<string> newItems(0);
	newItems.reserve(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i != index)
		{
			newItems.push_back(items[i]);
		}
	}
	items = newItems;

	if (items.empty())
	{
		selected = -1;
	}
	else if (selected > index)
	{
		selected--;
	}
}
void wiComboBox::ClearItems()
{
	items.clear();

	selected = -1;
}
void wiComboBox::SetMaxVisibleItemCount(int value)
{
	maxVisibleItemCount = value;
}
void wiComboBox::SetSelected(int index)
{
	selected = index;

	wiEventArgs args;
	args.iValue = selected;
	args.sValue = GetItemText(selected);
	onSelect(args);
}
string wiComboBox::GetItemText(int index)
{
	if (index >= 0)
	{
		return items[index];
	}
	return "";
}
int wiComboBox::GetSelected()
{
	return selected;
}




static const float windowcontrolSize = 20.0f;
wiWindow::wiWindow(wiGUI* gui, const std::string& name) :wiWidget()
, gui(gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(640, 480));

	// Add controls

	SAFE_INIT(closeButton);
	SAFE_INIT(moveDragger);
	SAFE_INIT(resizeDragger_BottomRight);
	SAFE_INIT(resizeDragger_UpperLeft);



	// Add a grabber onto the title bar
	moveDragger = new wiButton(name + "_move_dragger");
	moveDragger->SetText("");
	moveDragger->SetSize(XMFLOAT2(scale.x - windowcontrolSize * 3, windowcontrolSize));
	moveDragger->SetPos(XMFLOAT2(windowcontrolSize, 0));
	moveDragger->OnDrag([this](wiEventArgs args) {
		this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
	});
	AddWidget(moveDragger);

	// Add close button to the top right corner
	closeButton = new wiButton(name + "_close_button");
	closeButton->SetText("x");
	closeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	closeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y));
	closeButton->OnClick([this](wiEventArgs args) {
		this->SetVisible(false);
	});
	closeButton->SetTooltip("Close window");
	AddWidget(closeButton);

	// Add minimize button to the top right corner
	minimizeButton = new wiButton(name + "_minimize_button");
	minimizeButton->SetText("-");
	minimizeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	minimizeButton->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize *2, translation.y));
	minimizeButton->OnClick([this](wiEventArgs args) {
		this->SetMinimized(!this->IsMinimized());
	});
	minimizeButton->SetTooltip("Minimize window");
	AddWidget(minimizeButton);

	// Add a resizer control to the upperleft corner
	resizeDragger_UpperLeft = new wiButton(name + "_resize_dragger_upper_left");
	resizeDragger_UpperLeft->SetText("");
	resizeDragger_UpperLeft->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	resizeDragger_UpperLeft->SetPos(XMFLOAT2(0, 0));
	resizeDragger_UpperLeft->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (scale.x - args.deltaPos.x) / scale.x;
		scaleDiff.y = (scale.y - args.deltaPos.y) / scale.y;
		this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
		this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	AddWidget(resizeDragger_UpperLeft);

	// Add a resizer control to the bottom right corner
	resizeDragger_BottomRight = new wiButton(name + "_resize_dragger_bottom_right");
	resizeDragger_BottomRight->SetText("");
	resizeDragger_BottomRight->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	resizeDragger_BottomRight->SetPos(XMFLOAT2(translation.x + scale.x - windowcontrolSize, translation.y + scale.y - windowcontrolSize));
	resizeDragger_BottomRight->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (scale.x + args.deltaPos.x) / scale.x;
		scaleDiff.y = (scale.y + args.deltaPos.y) / scale.y;
		this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	AddWidget(resizeDragger_BottomRight);


	SetEnabled(true);
	SetVisible(true);
	SetMinimized(false);
}
wiWindow::~wiWindow()
{
	RemoveWidgets(true);
}
void wiWindow::AddWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	widget->SetEnabled(this->IsEnabled());
	widget->SetVisible(this->IsVisible());
	gui->AddWidget(widget);
	widget->attachTo(this);
	widget->container = this;

	childrenWidgets.push_back(widget);
}
void wiWindow::RemoveWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->RemoveWidget(widget);
	widget->detach();

	childrenWidgets.remove(widget);
}
void wiWindow::RemoveWidgets(bool alsoDelete)
{
	assert(gui != nullptr && "Ivalid GUI!");

	for (auto& x : childrenWidgets)
	{
		x->detach();
		gui->RemoveWidget(x);
		if (alsoDelete)
		{
			SAFE_DELETE(x);
		}
	}

	childrenWidgets.clear();
}
void wiWindow::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	//// TODO: Override window controls for nicer alignment:
	//if (moveDragger != nullptr)
	//{
	//	moveDragger->scale.y = windowcontrolSize;
	//}
	//if (resizeDragger_UpperLeft != nullptr)
	//{
	//	resizeDragger_UpperLeft->scale.x = windowcontrolSize;
	//	resizeDragger_UpperLeft->scale.y = windowcontrolSize;
	//}
	//if (resizeDragger_BottomRight != nullptr)
	//{
	//	resizeDragger_BottomRight->scale.x = windowcontrolSize;
	//	resizeDragger_BottomRight->scale.y = windowcontrolSize;
	//}
	//if (closeButton != nullptr)
	//{
	//	closeButton->scale.x = windowcontrolSize;
	//	closeButton->scale.y = windowcontrolSize;
	//}
	//if (minimizeButton != nullptr)
	//{
	//	minimizeButton->scale.x = windowcontrolSize;
	//	minimizeButton->scale.y = windowcontrolSize;
	//}


	if (!IsEnabled())
	{
		return;
	}

	for (auto& x : childrenWidgets)
	{
		x->Update(gui, dt);
		x->SetScissorRect(scissorRect);
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiWindow::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	// body
	if (!IsMinimized())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
			, wiImageEffects(translation.x, translation.y, scale.x, scale.y), gui->GetGraphicsThread());
	}

	for (auto& x : childrenWidgets)
	{
		if (x != gui->GetActiveWidget())
		{
			// the gui will render the active on on top of everything!
			x->Render(gui);
		}
	}

	scissorRect.bottom = (LONG)(translation.y + scale.y);
	scissorRect.left = (LONG)(translation.x);
	scissorRect.right = (LONG)(translation.x + scale.x);
	scissorRect.top = (LONG)(translation.y);
	wiRenderer::GetDevice()->SetScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)(translation.x + resizeDragger_UpperLeft->scale.x + 2), (int)(translation.y), -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread(), true);

}
void wiWindow::SetVisible(bool value)
{
	wiWidget::SetVisible(value);
	SetMinimized(!value);
	for (auto& x : childrenWidgets)
	{
		x->SetVisible(value);
	}
}
void wiWindow::SetEnabled(bool value)
{
	//wiWidget::SetEnabled(value);
	for (auto& x : childrenWidgets)
	{
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
		x->SetEnabled(value);
	}
}
void wiWindow::SetMinimized(bool value)
{
	minimized = value;

	if (resizeDragger_BottomRight != nullptr)
	{
		resizeDragger_BottomRight->SetVisible(!value);
	}
	for (auto& x : childrenWidgets)
	{
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
		x->SetVisible(!value);
	}
}
bool wiWindow::IsMinimized()
{
	return minimized;
}




wiColorPicker::wiColorPicker(wiGUI* gui, const std::string& name) :wiWindow(gui, name)
{
	SetSize(XMFLOAT2(300, 260));
	SetColor(wiColor::Ghost);
	RemoveWidget(resizeDragger_BottomRight);
	RemoveWidget(resizeDragger_UpperLeft);

	hue_picker = XMFLOAT2(0, 0);
	saturation_picker = XMFLOAT2(0, 0);
	saturation_picker_barycentric = XMFLOAT3(0, 1, 0);
	hue_color = XMFLOAT4(1, 0, 0, 1);
	final_color = XMFLOAT4(1, 1, 1, 1);
	angle = 0;
}
wiColorPicker::~wiColorPicker()
{
}
static const float __colorpicker_center = 120;
static const float __colorpicker_radius_triangle = 75;
static const float __colorpicker_radius = 80;
static const float __colorpicker_width = 16;
void wiColorPicker::Update(wiGUI* gui, float dt )
{
	wiWindow::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	//if (!gui->IsWidgetDisabled(this))
	//{
	//	return;
	//}

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	XMFLOAT2 center = XMFLOAT2(translation.x + __colorpicker_center, translation.y + __colorpicker_center);
	XMFLOAT2 pointer = gui->GetPointerPos();
	float distance = wiMath::Distance(center, pointer);
	bool hover_hue = (distance > __colorpicker_radius) && (distance < __colorpicker_radius + __colorpicker_width);

	float distTri = 0;
	XMFLOAT4 A, B, C;
	wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, A, B, C);
	XMVECTOR _A = XMLoadFloat4(&A);
	XMVECTOR _B = XMLoadFloat4(&B);
	XMVECTOR _C = XMLoadFloat4(&C);
	XMMATRIX _triTransform = XMMatrixRotationZ(-angle) * XMMatrixTranslation(center.x, center.y, 0);
	_A = XMVector4Transform(_A, _triTransform);
	_B = XMVector4Transform(_B, _triTransform);
	_C = XMVector4Transform(_C, _triTransform);
	XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
	XMVECTOR D = XMVectorSet(0, 0, 1, 0);
	bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

	if (hover_hue && state==IDLE)
	{
		state = FOCUS;
		huefocus = true;
	}
	else if (hover_saturation && state == IDLE)
	{
		state = FOCUS;
		huefocus = false;
	}
	else if (state == IDLE)
	{
		huefocus = false;
	}

	bool dragged = false;

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == ACTIVE)
		{
			// continue drag if already grabbed wheter it is intersecting or not
			dragged = true;
		}
	}

	dragged = dragged && !gui->IsWidgetDisabled(this);
	if (huefocus && dragged)
	{
		//hue pick
		angle = wiMath::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(__colorpicker_radius, 0));
		XMFLOAT3 color3 = wiMath::HueToRGB(angle / XM_2PI);
		hue_color = XMFLOAT4(color3.x, color3.y, color3.z, 1);
		gui->ActivateWidget(this);
	}
	else if (!huefocus && dragged)
	{
		// saturation pick
		wiMath::GetBarycentric(O, _A, _B, _C, saturation_picker_barycentric.x, saturation_picker_barycentric.y, saturation_picker_barycentric.z, true);
		gui->ActivateWidget(this);
	}
	else if(state != IDLE)
	{
		gui->DeactivateWidget(this);
	}

	float r = __colorpicker_radius + __colorpicker_width*0.5f;
	hue_picker = XMFLOAT2(center.x + r*cos(angle), center.y + r*-sin(angle));
	XMStoreFloat2(&saturation_picker, _A*saturation_picker_barycentric.x + _B*saturation_picker_barycentric.y + _C*saturation_picker_barycentric.z);
	XMStoreFloat4(&final_color, XMLoadFloat4(&hue_color)*saturation_picker_barycentric.x + XMVectorSet(1, 1, 1, 1)*saturation_picker_barycentric.y + XMVectorSet(0, 0, 0, 1)*saturation_picker_barycentric.z);

	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointer;
		args.fValue = angle;
		args.color = final_color;
		onColorChanged(args);
	}
}
void wiColorPicker::Render(wiGUI* gui)
{
	wiWindow::Render(gui);


	if (!IsVisible() || IsMinimized())
	{
		return;
	}
	GRAPHICSTHREAD threadID = gui->GetGraphicsThread();

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static wiGraphicsTypes::GPUBuffer vb_saturation;
	static wiGraphicsTypes::GPUBuffer vb_hue;
	static wiGraphicsTypes::GPUBuffer vb_picker;
	static wiGraphicsTypes::GPUBuffer vb_preview;

	static bool buffersComplete = false;
	if (!buffersComplete)
	{
		HRESULT hr = S_OK;
		// saturation
		{
			static std::vector<Vertex> vertices(0);
			if (vb_saturation.IsValid() && !vertices.empty())
			{
				vertices[0].col = hue_color;
				wiRenderer::GetDevice()->UpdateBuffer(&vb_saturation, vertices.data(), threadID, vb_saturation.GetDesc().ByteWidth);
			}
			else
			{
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
				vertices.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
				wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, vertices[0].pos, vertices[1].pos, vertices[2].pos);

				GPUBufferDesc desc;
				desc.BindFlags = BIND_VERTEX_BUFFER;
				desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
				desc.CPUAccessFlags = CPU_ACCESS_WRITE;
				desc.MiscFlags = 0;
				desc.StructureByteStride = 0;
				desc.Usage = USAGE_DYNAMIC;
				SubresourceData data;
				data.pSysMem = vertices.data();
				hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_saturation);
			}
		}
		// hue
		{
			std::vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				XMFLOAT3 rgb = wiMath::HueToRGB(p);
				vertices.push_back({ XMFLOAT4(__colorpicker_radius * x, __colorpicker_radius * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
				vertices.push_back({ XMFLOAT4((__colorpicker_radius + __colorpicker_width) * x, (__colorpicker_radius + __colorpicker_width) * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_DYNAMIC;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_hue);
		}
		// picker
		{
			float _radius = 3;
			float _width = 3;
			std::vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				vertices.push_back({ XMFLOAT4(_radius * x, _radius * y, 0, 1), XMFLOAT4(1,1,1,1) });
				vertices.push_back({ XMFLOAT4((_radius + _width) * x, (_radius + _width) * y, 0, 1), XMFLOAT4(1,1,1,1) });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_picker);
		}
		// preview
		{
			float _width = 20;

			std::vector<Vertex> vertices(0);
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_preview);
		}

	}

	XMMATRIX __cam = wiRenderer::GetDevice()->GetScreenProjection();

	wiRenderer::GetDevice()->BindConstantBufferVS(wiRenderer::constantBuffers[CBTYPE_MISC], CBSLOT_RENDERER_MISC, threadID);
	wiRenderer::GetDevice()->BindRasterizerState(wiRenderer::rasterizers[RSTYPE_DOUBLESIDED], threadID);
	wiRenderer::GetDevice()->BindBlendState(wiRenderer::blendStates[BSTYPE_OPAQUE], threadID);
	wiRenderer::GetDevice()->BindDepthStencilState(wiRenderer::depthStencils[DSSTYPE_XRAY], 0, threadID);
	wiRenderer::GetDevice()->BindVertexLayout(wiRenderer::vertexLayouts[VLTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindVS(wiRenderer::vertexShaders[VSTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindPS(wiRenderer::pixelShaders[PSTYPE_LINE], threadID);
	wiRenderer::GetDevice()->BindPrimitiveTopology(TRIANGLESTRIP, threadID);

	wiRenderer::MiscCB cb;

	// render saturation triangle
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixRotationZ(-angle) *
			XMMatrixTranslation(translation.x + __colorpicker_center, translation.y + __colorpicker_center, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1, 1, 1, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		const GPUBuffer* vbs[] = {
			&vb_saturation,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_saturation.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render hue circle
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(translation.x + __colorpicker_center, translation.y + __colorpicker_center, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1, 1, 1, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		const GPUBuffer* vbs[] = {
			&vb_hue,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render hue picker
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(hue_picker.x, hue_picker.y, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1 - hue_color.x, 1 - hue_color.y, 1 - hue_color.z, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		const GPUBuffer* vbs[] = {
			&vb_picker,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render saturation picker
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(saturation_picker.x, saturation_picker.y, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1 - final_color.x, 1 - final_color.y, 1 - final_color.z, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		const GPUBuffer* vbs[] = {
			&vb_picker,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render preview
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(translation.x + 260, translation.y + 40, 0) *
			__cam
		);
		cb.mColor = GetPickColor();
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		const GPUBuffer* vbs[] = {
			&vb_preview,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_preview.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// RGB values:
	stringstream _rgb("");
	_rgb << "R: " << (int)(final_color.x * 255) << endl;
	_rgb << "G: " << (int)(final_color.y * 255) << endl;
	_rgb << "B: " << (int)(final_color.z * 255) << endl;
	wiFont(_rgb.str(), wiFontProps((int)(translation.x + 200), (int)(translation.y + 200), -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1,
		textColor, textShadowColor)).Draw(threadID, true);
	
}
XMFLOAT4 wiColorPicker::GetPickColor()
{
	return final_color;
}
void wiColorPicker::SetPickColor(const XMFLOAT4& value)
{
	// TODO
}
void wiColorPicker::OnColorChanged(function<void(wiEventArgs args)> func)
{
	onColorChanged = move(func);
}
