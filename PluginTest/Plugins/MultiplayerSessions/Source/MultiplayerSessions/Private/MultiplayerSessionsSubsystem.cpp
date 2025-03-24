// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include <Online/OnlineSessionNames.h>

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	//��ʼ��ί���¼�
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	//��������ϵͳ�л�ȡ���߻Ự�ӿ�
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		SessionInterface = Subsystem->GetSessionInterface();
	}
}

//
//�����Ự
//


void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	//���߻Ự�ӿ���Чֱ�ӷ���
	if (!SessionInterface.IsValid())
	{
		return;
	}

	//�������ִ�ĻỰ���򱣴湫���������ͱ������ͺ����ٻỰ
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	// ��ί�д洢�� FDelegateHandle ���Ա����ɾ��
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	//�Ự����
	
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;//������;����
	LastSessionSettings->bAllowJoinViaPresence = true;//������ҿ���ͨ���鿴�������״̬�����硰���ߡ�����Ϸ�С�����ֱ�Ӽ��������Ϸ�Ự��
	LastSessionSettings->bShouldAdvertise = true;//��Ϸ�Ự�ᱻ������������ҿ���ͨ���Ự���������ҵ�������ûỰ��
	LastSessionSettings->bUsesPresence = true;//��Ϸ�Ự��ʹ������״̬���ܣ���ҵ�״̬��Ϣ�ᱻ���²�����ɼ���
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);//���߻Ự������ֵ�ԣ����ƣ��������ͣ��Ự�㲥��ʽ��
	LastSessionSettings->BuildUniqueId = 1;//�ỰΨһID,ȷ�����������绷���о���Ψһ��,�����ͻ
	LastSessionSettings->bUseLobbiesIfAvailable = true;//�Ƿ�ʹ��ƽ̨�ġ���������Lobby��������������Ϸ�Ự

	//ͨ��Ψһ���ID�����Ự���紴��ʧ�����ί�в������Զ���ί��
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		//�ಥί�з��ش���ʧ�ܣ����¼���Menu�Ĺ���ʱ��
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}


//
//�����Ự
//

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	//�Ự�ӿ���Ч���˳�
	if (!SessionInterface.IsValid())
	{
		return;
	}
	//���������Ựί�о��
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;//��������������
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;//�Ƿ�Ϊ������������
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);//��Ҫ���������� ������״̬  ���Ƚϲ�������

	//�����Ự����Ự����������������Ựί�о��
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		//�ಥ�����Ựʧ��
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}


//
//����Ự
//

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	//�ӿ���Ч��ಥ����Ựʧ��
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	//��������Ự���ί�о��
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);


	//����Ự����ʧ�����������Ự������ಥ����Ựʧ��
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

//
//���ٻỰ
//

void UMultiplayerSessionsSubsystem::DestroySession()
{
	//�ӿ���Ч��ಥ���ٻỰʧ��
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	//��������Ự���ί�о��
	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	//���ٻỰ����ʧ����������ٻỰ������ಥ���ٻỰʧ��
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
}

//
//�����Ự���ʱ
//

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	//��������Ự���ʱ���
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	//�ಥ�����Ự�ɹ�
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

//
//�����Ự���ʱ
//

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	//��������Ự���ʱ���
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	//û�������������ಥ�����Ựʧ��
	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	//�ಥ�����Ự�ɹ�
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

//
//����Ự���ʱ
//

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//�������Ự���ʱ���
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	//�ಥ����Ự�ɹ������ؽ��
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

//
//���ٻỰ���ʱ
//

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	//������ٻỰ���ʱ���
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	//�����ٺ󴴽��Ự
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	//�ಥ���ٻỰ�ɹ�
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
}
