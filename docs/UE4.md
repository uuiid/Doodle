>前缀 "A" 代表它是 Actor 的子类，而前缀 "U" 代表它是 Object 的子类。还有其他一些前缀，比如 "F" 通常表示一个简单的数据结构类，或者其他非 Uboject 类。
### 接口
接口类使用UINTERFACE宏,直接从"UInterface"继承，
前缀为U（U-prefixed）"的类不需要构造函数或任何其他函数，而"前缀为I（I-prefixed）"的类将包含所有接口函数，且此类实际上将被您的其他类继承。  

|接口说明符           |     含义                      |
| -------------------| ------------------------------|
|"BlueprintType"     |   将该类公开为可用于蓝图中的变量的类型。|
|"DependsOn=(ClassName1, ClassName2, ...)"|所有列出的类都将在该类之前编译。|///?为每个类指定
|"MinimalAPI"        |仅导致该类的类型信息被导出以供其他模块使用。您可以向该类转换，但不能调用该类的函数（内联方法除外）。|  
### 为了与实现接口的C++和蓝图类兼容，可以使用以下任意函数
bool bIsImplemented = OriginalObject->GetClass()->ImplementsInterface(UReactToTriggerInterface::StaticClass());   
// 如果OriginalObject实现了UReactToTriggerInterface，则bisimplemated将为true。  


bIsImplemented = OriginalObject->Implements<UReactToTriggerInterface>();   
// 如果OriginalObject实现了UReactToTrigger，bIsImplemented将为true。  


IReactToTriggerInterface* ReactingObject = Cast<IReactToTriggerInterface>(OriginalObject);  
 // 如果OriginalObject实现了UReactToTriggerInterface，则ReactingObject将为非空。   
 ### 转换到其他虚幻类型
 IReactToTriggerInterface* ReactingObject = Cast<IReactToTriggerInterface>(OriginalObject);  
  // 如果接口被实现，则ReactingObject将为非空。

ISomeOtherInterface* DifferentInterface = Cast<ISomeOtherInterface>(ReactingObject);  
 // 如果ReactingObject为非空而且还实现了ISomeOtherInterface，则DifferentInterface将为非空。

AActor* Actor = Cast<AActor>(ReactingObject);   
// 如果ReactingObject为非空且OriginalObject为AActor或AActor派生的类，则Actor将为非空。   
>KeepRelativeTransform（保持两个物体之间的相对位置不变）  
KeepWorldTransform（保持连个物体在世界的位置不变）
SnapToTargetNotIncludingScale（保持物体的缩放对齐到目标上）
SnapToTargetIncludingScale（随目标的缩放）


## ue4 发布构建
Engine\Build\BatchFiles\RunUAT.bat BuildGraph -target="Make Installed Build Win64" -script=Engine/Build/InstalledEngineBuild.xml -set:HostPlatformOnly=true -set:WithDDC=false -set:GameConfigurations=Development;Shipping -set:VS2019=true
或者加入 
Engine\Build\BatchFiles\RunUAT.bat BuildGraph -target="Make Installed Build Win64" -script=Engine/Build/InstalledEngineBuild.xml -clean -set:HostPlatformOnly=true -set:WithDDC=false
其中windows sdk 必须是 Windows 10.0.18362.0 SDK, 这个是针对 rtx-dlss-4.27分支的
