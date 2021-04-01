#include "DoodleToolPalette.h"
#include "SlateBasics.h"


//#include "IPlacementModeModule.h"
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDoodleToolPalette::Construct(const FArguments& InArgs)
{
	this->ChildSlot[
		SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)[
				SNew(SButton)
			]

	];

	SAssignNew(DoodleClassList, SListView<TSharedPtr<SDoodleToolListItem>>)
		.SelectionMode(ESelectionMode::Single)
		.ListItemsSource({});

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
