#include "QGUIProgressCtrl.h"

#include <QGroupBox>
#include <QGridLayout>

/* QGUIProgressCtrl */
QGUIProgressCtrl::QGUIProgressCtrl(plCreatable* pCre, QWidget* parent)
                : QCreatable(pCre, kGUIProgressCtrl, parent)
{
    pfGUIProgressCtrl* ctrl = (pfGUIProgressCtrl*)fCreatable;

    fControlModLink = new QCreatableLink(this, false);
    fControlModLink->setText(tr("GUI Control Common Properties"));
    fControlModLink->setCreatable(ctrl);
    fControlModLink->setForceType(kGUIControlMod);

    QGroupBox* grpFlags = new QGroupBox(tr("Flags"), this);
    fModFlags[pfGUIProgressCtrl::kReverseValues - kModFlagStart] = new QCheckBox(tr("Reverse Values"), grpFlags);
    fModFlags[pfGUIProgressCtrl::kLeftRightOrientation - kModFlagStart] = new QCheckBox(tr("Left-Right Orientation"), grpFlags);
    fModFlags[pfGUIProgressCtrl::kMapToScreenRange - kModFlagStart] = new QCheckBox(tr("Map to Screen Range"), grpFlags);
    fModFlags[pfGUIProgressCtrl::kTriggerOnlyOnMouseUp - kModFlagStart] = new QCheckBox(tr("Trigger Only on MouseUp"), grpFlags);
    fModFlags[pfGUIProgressCtrl::kMapToAnimationRange - kModFlagStart] = new QCheckBox(tr("Map to Anim Range"), grpFlags);
    QGridLayout* flagLayout = new QGridLayout(grpFlags);
    flagLayout->setContentsMargins(8, 8, 8, 8);
    flagLayout->setVerticalSpacing(0);
    flagLayout->setHorizontalSpacing(8);
    flagLayout->addWidget(fModFlags[pfGUIProgressCtrl::kReverseValues - kModFlagStart], 0, 0);
    flagLayout->addWidget(fModFlags[pfGUIProgressCtrl::kLeftRightOrientation - kModFlagStart], 1, 0);
    flagLayout->addWidget(fModFlags[pfGUIProgressCtrl::kMapToScreenRange - kModFlagStart], 2, 0);
    flagLayout->addWidget(fModFlags[pfGUIProgressCtrl::kTriggerOnlyOnMouseUp - kModFlagStart], 3, 0);
    flagLayout->addWidget(fModFlags[pfGUIProgressCtrl::kMapToAnimationRange - kModFlagStart], 4, 0);
    for (size_t i=0; i<kModFlagCount; i++)
        fModFlags[i]->setChecked(ctrl->getFlag(i + kModFlagStart));

    fMin = new QFloatEdit(this);
    fMin->setRange(-2147483648.0, 2147483647.0, 3);
    fMin->setValue(ctrl->getMin());
    fMax = new QFloatEdit(this);
    fMax->setRange(-2147483648.0, 2147483647.0, 3);
    fMax->setValue(ctrl->getMax());
    fStep = new QFloatEdit(this);
    fStep->setRange(-2147483648.0, 2147483647.0, 3);
    fStep->setValue(ctrl->getStep());

    fAnimName = new QLineEdit(this);
    fAnimName->setText(~ctrl->getAnimName());
    fAnimationKeys = new QKeyList(this);
    for (size_t i=0; i<ctrl->getNumAnimKeys(); i++)
        fAnimationKeys->addKey(ctrl->getAnimKey(i));

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(fControlModLink, 0, 0, 1, 3);
    layout->addWidget(grpFlags, 1, 0, 1, 3);
    layout->addWidget(new QLabel(tr("Range:"), this), 2, 0);
    layout->addWidget(fMin, 2, 1);
    layout->addWidget(fMax, 2, 2);
    layout->addWidget(new QLabel(tr("Step:"), this), 3, 0);
    layout->addWidget(fStep, 3, 1);
    layout->addWidget(new QLabel(tr("Anim Name:"), this), 4, 0);
    layout->addWidget(fAnimName, 4, 1, 1, 2);
    layout->addWidget(fAnimationKeys, 5, 0, 1, 3);

    fAnimationKeys->adjustColumns();
}

void QGUIProgressCtrl::saveDamage()
{
    pfGUIProgressCtrl* ctrl = (pfGUIProgressCtrl*)fCreatable;

    for (size_t i=0; i<kModFlagCount; i++) {
        if (fModFlags[i]->isChecked())
            ctrl->setFlag(i + kModFlagStart);
        else
            ctrl->clearFlag(i + kModFlagStart);
    }

    ctrl->setRange(fMin->value(), fMax->value());
    ctrl->setStep(fStep->value());

    ctrl->setAnimName(~fAnimName->text());
    ctrl->clearAnimKeys();
    QList<plKey> animKeys = fAnimationKeys->keys();
    while (!animKeys.isEmpty())
        ctrl->addAnimKey(animKeys.takeFirst());
}