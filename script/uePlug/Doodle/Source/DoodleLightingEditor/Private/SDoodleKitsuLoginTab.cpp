#include "SDoodleKitsuLoginTab.h"

#include "SDoodleKitsuLoginPanel.h"

#include "Widgets/Docking/SDockTab.h"

const FName SDoodleKitsuLoginTab::TabName{ TEXT("DoodleKitsuLoginTab") };

void SDoodleKitsuLoginTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SDoodleKitsuLoginPanel)
	];
}

TSharedRef<SDockTab> SDoodleKitsuLoginTab::OnSpawnAction(const FSpawnTabArgs& InSpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDoodleKitsuLoginTab)
		];
}
