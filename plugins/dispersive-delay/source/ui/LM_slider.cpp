#include "LM_slider.h"

void L_MODEL_STYLE::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider)
{
}

void L_MODEL_STYLE::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
	juce::Path arcPath1, arcPath2;
	float zoomVal = 0.55;//放大系数
	arcPath1.addArc(x + width * (1.0 - zoomVal) / 2.0, y + height * (1.0 - zoomVal) / 2.0 - 4, width * zoomVal, height * zoomVal, M_PI / 4 - M_PI, M_PI / 4 - M_PI + sliderPosProportional * M_PI * 1.5, true);
	arcPath2.addArc(x + width * (1.0 - zoomVal) / 2.0, y + height * (1.0 - zoomVal) / 2.0 - 4, width * zoomVal, height * zoomVal, M_PI / 4 - M_PI + sliderPosProportional * M_PI * 1.5, -M_PI / 4 + M_PI, true);
	g.setColour(juce::Colour(0x66, 0x66, 0xCC));
	g.strokePath(arcPath1, juce::PathStrokeType(4.0));//绘制，这里才是设置线宽
	g.setColour(juce::Colour(0x33, 0x33, 0x66));
	g.strokePath(arcPath2, juce::PathStrokeType(4.0));

	g.setColour(juce::Colour(0x22, 0xff, 0x22));//画绿色的线
	float rotx = -sin(M_PI / 4 + sliderPosProportional * M_PI * 1.5), roty = cos(M_PI / 4 + sliderPosProportional * M_PI * 1.5);
	g.drawLine(x + width / 2 + rotx * 11 * zoomVal * 2.0,
		y + height / 2 + roty * 11 * zoomVal * 2.0 - 4,
		x + width / 2 + rotx * 17 * zoomVal * 2.0,
		y + height / 2 + roty * 17 * zoomVal * 2.0 - 4, 4.0);
}

void L_MODEL_STYLE::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	if (shouldDrawButtonAsHighlighted) g.setColour(juce::Colour(0x66, 0x66, 0xCC));
	else g.setColour(juce::Colour(0x33, 0x33, 0x66));

	juce::Rectangle<int> rect = button.getBounds();
	int x = rect.getX(), y = rect.getY(), w = rect.getWidth(), h = rect.getHeight();
	g.drawLine(x, y, x + w, y, 4);
	g.drawLine(x, y, x, y + h, 4);
	g.drawLine(x + w, y + h, x + w, y, 4);
	g.drawLine(x + w, y + h, x, y + h, 4);
}

void L_MODEL_STYLE::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
}

void L_MODEL_STYLE::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)

{
	juce::Rectangle<int> rect = button.getBounds();
	int x = rect.getX(), y = rect.getY(), w = rect.getWidth(), h = rect.getHeight();//获取按钮尺寸

	if (shouldDrawButtonAsDown || button.getToggleState())//如果按下
	{
		g.setColour(juce::Colour(0x33, 0x33, 0x66));
		g.fillRect(x, y, w, h);//画填充矩形
	}
	else if (shouldDrawButtonAsHighlighted)//如果鼠标靠近
	{
		g.setColour(juce::Colour(0x22, 0x22, 0x44));
		g.fillRect(x, y, w, h);//画填充矩形
	}//否则不画矩形

	g.setColour(juce::Colour(0x77, 0x77, 0xEE));
	g.drawLine(x, y, x + w, y, 4);//画框
	g.drawLine(x, y, x, y + h, 4);
	g.drawLine(x + w, y + h, x + w, y, 4);
	g.drawLine(x + w, y + h, x, y + h, 4);

	//绘制文字
	g.setColour(juce::Colour(0xff, 0xff, 0xff));
	juce::String name = button.getButtonText();
	g.setFont(juce::Font("FIXEDSYS", 15.0, 0));
	g.drawText(name, x, y, w, h, juce::Justification::centred);
}

void L_MODEL_STYLE::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonWidth, int buttonHeight, juce::ComboBox& box)
{
		int x = box.getX(), y = box.getY(), w = box.getWidth(), h = box.getHeight();
		if (isButtonDown)//如果鼠标靠近
		{
			g.setColour(juce::Colour(0x22, 0x22, 0x44));
			g.fillRect(x + 2, y, w - 4, h);//画填充矩形
		}

		g.setColour(juce::Colour(0x77, 0x77, 0xEE));
		g.drawLine(x, y, x + w, y, 4);//画框
		g.drawLine(x, y, x, y + h, 4);
		g.drawLine(x + w, y + h, x + w, y, 4);
		g.drawLine(x + w, y + h, x, y + h, 4);

		/*
		g.setColour(juce::Colour(0x77, 0xff, 0x77));
		juce::String name = box.getText();
		g.setFont(juce::Font("FIXEDSYS", 15.0, 0));
		g.drawText(name, x + 8, y, w - 8, h, juce::Justification::centredLeft);
		*/
}

void L_MODEL_STYLE::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, const bool isSeparator, const bool isActive, const bool isHighlighted, const bool isTicked, const bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour)
	{
		int x = area.getX(), y = area.getY(), w = area.getWidth(), h = area.getHeight();
		if (isTicked)//如果按下
		{
			g.setColour(juce::Colour(0x44, 0x44, 0x88));
			g.fillRect(x + 2, y, w - 4, h);//画填充矩形
		}
		else if (isHighlighted)//如果鼠标靠近
		{
			g.setColour(juce::Colour(0x22, 0x22, 0x44));
			g.fillRect(x + 2, y, w - 4, h);//画填充矩形
		}

		g.setColour(juce::Colour(0x77, 0xff, 0x77));
		g.setFont(juce::Font("FIXEDSYS", 15.0, 0));
		g.drawText(text, x + 8, y, w - 8, h, juce::Justification::centredLeft);
}

void L_MODEL_STYLE::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
	g.fillAll(juce::Colour(0x11, 0x11, 0x22));//黑的
	g.setColour(juce::Colour(0x44, 0x44, 0x88));
	g.drawLine(0, 0, width, 0, 4);
	g.drawLine(0, 0, 0, height, 4);
	g.drawLine(width, height, width, 0, 4);
	g.drawLine(width, height, 0, height, 4);
}

void Custom1_Slider::mouseDown(const juce::MouseEvent& event)
{
	lastMousePosition = event.source.getScreenPosition();
	setMouseCursor(juce::MouseCursor::NoCursor);
	juce::Slider::mouseDown(event);
}

void Custom1_Slider::mouseUp(const juce::MouseEvent& event)
{
	juce::Desktop::getInstance().getMainMouseSource().setScreenPosition(lastMousePosition);
	setMouseCursor(juce::MouseCursor::NormalCursor);
	juce::Slider::mouseUp(event);
}

LMKnob::LMKnob() :slider(), label()
{
	slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
	slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	L_MODEL_STYLE_LOOKANDFEEL = std::make_unique<L_MODEL_STYLE>();
	slider.setLookAndFeel(L_MODEL_STYLE_LOOKANDFEEL.get());//应用l-model风格
	label.setJustificationType(juce::Justification::centredTop);
	label.setFont(juce::Font("FIXEDSYS", 15.0, 0));
	label.setMinimumHorizontalScale(1.0);//不缩放字体
	label.setColour(juce::Label::textColourId, juce::Colour(0x77, 0xff, 0x77));
	label.setFont(label.getFont().withStyle(juce::Font::bold));//设置粗体
	/*
	slider.onValueChange = [this]
	{
		this->repaint(-64, -64, 128, 128);
	};
	slider.setInterceptsMouseClicks(true, true);//使得slider在组件外的区域也能接收鼠标事件
	*/
	setPaintingIsUnclipped(true);//组件无边界
	setOpaque(false);//组件黑色部分透明

	addAndMakeVisible(slider);
	addAndMakeVisible(label);
}

LMKnob::~LMKnob()
{
	slider.setLookAndFeel(nullptr);
	ParamLinker = nullptr;
}

void LMKnob::paint(juce::Graphics& g)
{
	/*
	auto centreX = getWidth() / 2;//设置坐标原点
	auto centreY = getHeight() / 2;
	g.setOrigin(-centreX, -centreY);*/

	/*
	g.setColour(juce::Colour(0x00, 0xff, 0x00));//碰撞箱
	g.drawLine(0, 0, 64, 0);
	g.drawLine(0, 0, 0, 80);
	g.drawLine(64, 80, 64, 0);
	g.drawLine(64, 80, 0, 80);*/
}

void LMKnob::ParamLink(juce::AudioProcessorValueTreeState& stateToUse, const juce::String& parameterID)
{
	ParamLinker = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(stateToUse, parameterID, slider);
}

void LMKnob::setText(const juce::String& KnobText)
{
	label.setText(KnobText, juce::dontSendNotification);
	text = KnobText;
}

void LMKnob::resized()
{
	slider.setBounds(32 - 56 / 2, 32 - 56 / 2, 56, 56);
	label.setBounds(-32, 48 - 4, 64 + 64, 16);
}

void LMKnob::setPos(int x, int y)
{
	setBounds(x - 32, y - 32, 64, 64);
}

LMButton::LMButton()
{
	// 设置按钮属性
	button.setButtonText(name);
	button.addListener(this); // 注册监听器以处理按钮点击

	L_MODEL_STYLE_LOOKANDFEEL = std::make_unique<L_MODEL_STYLE>();
	button.setLookAndFeel(L_MODEL_STYLE_LOOKANDFEEL.get());//应用l-model风格

	setPaintingIsUnclipped(true);//组件无边界
	setOpaque(false);//组件黑色部分透明

	addAndMakeVisible(button);
}

LMButton::~LMButton()
{
	button.setLookAndFeel(nullptr);
}

void LMButton::setName(juce::String ButtonName)
{
	name = ButtonName;
	button.setButtonText(name);
}


void LMButton::resized()
{
	int w = getBounds().getWidth(), h = getBounds().getHeight();
	button.setBounds(0,0,w,h);
}

/*
void LMButton::setPos(int x, int y)
{
	// 替代setBounds
	setBounds(x - Width / 2, y - 12, Width, 24);
}

void LMButton::setButtonWidth(int ButtonWidth)
{
	Width = ButtonWidth;
	button.setBounds(0, 0, Width, 24);
	setBounds(getX(), getY(), Width, 24);
	repaint();
}
*/

void LMButton::buttonClicked(juce::Button* clicked)
{
	if (clicked == &button)
	{

	}
}

void LMButton::setClickedCallback(std::function<void()> cbFunc)
{
	button.onClick = cbFunc;
}

int LMButton::getButtonState()
{
	return button.getToggleState();
}

// LMCombox implementation

LMCombox::LMCombox()
{
	comboBox.addListener(this);
	L_MODEL_STYLE_LOOKANDFEEL = std::make_unique<L_MODEL_STYLE>();
	comboBox.setLookAndFeel(L_MODEL_STYLE_LOOKANDFEEL.get()); // 应用l-model风格
	addAndMakeVisible(comboBox);
}

LMCombox::~LMCombox()
{
	comboBox.setLookAndFeel(nullptr);
}

void LMCombox::addItem(juce::String name, int ID)
{
	comboBox.addItem(name, ID);
}

void LMCombox::setSelectedID(int ID)
{
	comboBox.setSelectedId(ID);
}

void LMCombox::setPos(int x, int y)
{
	setBounds(x - Width / 2, y - 12, Width, 24);
}

void LMCombox::setComboxWidth(int ComboxWidth)
{
	Width = ComboxWidth;
	comboBox.setBounds(0, 0, Width, 24);
	setBounds(getX(), getY(), Width, 24);
}

void LMCombox::resized()
{
	// 设置ComboBox的位置和大小
	comboBox.setBounds(0, 0, Width, 24);
}

void LMCombox::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged == &comboBox)
	{
		// Handle combo box change
	}
}