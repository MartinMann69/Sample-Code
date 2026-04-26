using Steamworks;
using System;
using System.Collections;
using Unity.VisualScripting;
using UnityEngine;

public class NetActor : MonoBehaviour, IInteractable
{
	public enum LifeState
	{
		Alive,
		Downed,
		Dead
	}
	
	public event Action<float, float> OnHealthChanged;
	public event Action<LifeState> OnLifeStateChanged;
	public event Action<bool> OnReadyChanged;

	[field: SerializeField] public ActorProfileObject Profile { get; private set; }
	[SerializeField] private GameObject prototypeDropPrefab;
	[SerializeField] private PlayerSteamID playerSteamId;
	[SerializeField] private PlayerController playerController;
	[SerializeField] private AnimationHandler animationHandler;
	[SerializeField] private Inventory inventory;
	[SerializeField] private ProceduralAnimationController proceduralAnimationController;
	[SerializeField] private Shot shot;
	[SerializeField] private bool godMode;
	
	[Header("Match Timer")]
	[SerializeField] private float matchTimerDuration = 10f; //! default lobby countdown
	[SerializeField] private bool matchTimerRunning;

	private PlayerProfileObject playerProfile;
	private Coroutine downedTimerCo;
	private Coroutine reviveCo;
	private Coroutine matchTimerCo;
	
	public bool IsLocalPlayer { get; private set; }
	public bool IsReady { get; private set; }
	public LifeState ActorLifeState { get; private set; }
	public float ActorHealthAmount { get; private set; }
	public float CurrentDownedTimer { get; private set; }
	public float CurrentReviveTimer { get; private set; }
	public float MatchTimerRemaining { get; private set; }
	public float CurrentHealth { get; private set; }
	public LifeState State { get; private set; } = LifeState.Alive;
	public bool IsPlayer => Profile != null && Profile.ActorKind == ActorKind.Player;
	public int Priority => 100;
	public bool Ready { get; private set; }
	
	string IInteractable.interactPrompt => "Revive";

	#region Event Functions
	private void Awake()
	{
		if (Profile == null)
		{
			Debug.LogError("[NetActor] Profile is not assigned.", this);
			return;
		}

		playerProfile = Profile as PlayerProfileObject;
	}

	private void OnEnable()
	{
		if (Profile == null)
		{
			Debug.LogError("[NetActor] Profile is not assigned.", this);
			return;
		}

		CurrentHealth = Mathf.Max(1, Profile.MaxHealth);
		ActorHealthAmount = CurrentHealth;
		ActorLifeState = State;

		if (playerSteamId != null)
		{
			IsLocalPlayer = playerSteamId.SteamID == SteamworksManagerASFR.instance.GetSteamID(true);
		}

		if (IsLocalPlayer)
		{
			SteamworksManagerASFR.instance.OnClientEvent("ActorHitOnClient", HitOnClientEvent);
			SteamworksManagerASFR.instance.OnServerEvent("ActorHitOnServer", HitOnServerEvent);

			SteamworksManagerASFR.instance.OnClientEvent("ReviveOnClient", ReviveOnClientEvent);
			SteamworksManagerASFR.instance.OnServerEvent("ReviveOnServer", ReviveOnServerEvent);

			SteamworksManagerASFR.instance.OnClientEvent("ActorDiedOnClient", ActorDiedOnClientEvent);
			SteamworksManagerASFR.instance.OnServerEvent("ActorDiedOnServer", ActorDiedOnServerEvent);

			SteamworksManagerASFR.instance.OnClientEvent("ActorReadyOnClient", ActorReadyOnClientEvent);
			SteamworksManagerASFR.instance.OnServerEvent("ActorReadyOnServer", ActorReadyOnServerEvent);

			SteamworksManagerASFR.instance.OnClientEvent("MatchTimerOnClient", MatchTimerOnClientEvent);
			SteamworksManagerASFR.instance.OnServerEvent("MatchTimerOnServer", MatchTimerOnServerEvent);
		}
	}
	#endregion

	#region Taking Damage
	private void HitOnClientEvent(byte[] payload)
	{
		(PacketID receivedId, IPacketData receivedData) = PacketManagerASFR.Deserialize(payload);
		if (receivedData is not ActorHitData hitPlayerData)
		{
			return;
		}

		// the if gets the player threw server info
		if (NetGameBootstrapper.PlayerObjects.TryGetValue(hitPlayerData.SenderID, out GameObject player))
		{
			player.GetComponent<NetActor>().TakeDamage(hitPlayerData.WeaponDamage);
		}
	}
	
	private void HitOnServerEvent(CSteamID steamID, byte[] payload)
	{
		HitOnClientEvent(payload);
		foreach (CSteamID member in SteamworksManagerASFR.instance.LobbyMembers)
		{
			//filter out Lobby owner
			if (member != SteamworksManagerASFR.instance.LobbyOwner)
			{
				SteamworksManagerASFR.instance.FireClient(member, "ActorHitOnClient", payload);
			}
		}
	}

	public void TakeDamage(float amount)
	{
		if (CurrentHealth <= 0 || State == LifeState.Dead || State == LifeState.Downed)
		{
			return;
		}

		if (godMode)
		{
			return;
		}

		CurrentHealth = Mathf.Max(0, CurrentHealth - amount);
		ActorHealthAmount = CurrentHealth;

		if (CurrentHealth == 0)
		{
			HandleZeroHealth();
		}

		OnHealthChanged?.Invoke(CurrentHealth, Profile.MaxHealth);
	}

	private void HandleZeroHealth()
	{
		if (!IsPlayer && State == LifeState.Downed)
		{
			return;
		}

		State = LifeState.Downed;

		if (playerController != null)
		{
			playerController.SetCanMove(false);
		}

		if (animationHandler != null)
		{
			animationHandler.RPC(nameof(animationHandler.SetDowned), true);
		}

		if (inventory != null && inventory.LootSlot != "None")
		{
			inventory.DropLoot(transform, prototypeDropPrefab);
		}

		if (MissionManager.instance != null)
		{
			MissionManager.instance.NotifyPlayerDowned(this);
		}

		ActorLifeState = State;
		OnLifeStateChanged?.Invoke(State);

		// Deequip weapon animation with longer duration for downed state
		// Use instance-based call so RPC syncs to all players
		if (proceduralAnimationController != null)
		{
			proceduralAnimationController.DeequipWeapon(instant: true);
		}

		downedTimerCo = StartCoroutine(DownedTimer(playerProfile.Revive.DownedWindowSeconds));
	}
	#endregion

	#region Heal
	public float Heal(float amount)
	{
		if (amount <= 0 || State != LifeState.Alive)
		{
			return 0;
		}

		float before = CurrentHealth;
		Debug.Log(before);
		CurrentHealth = Mathf.Min(Profile.MaxHealth, CurrentHealth + amount);
		ActorHealthAmount = CurrentHealth;
		Debug.Log(CurrentHealth);
		float healed = CurrentHealth - before;
		Debug.Log(healed);

		OnHealthChanged?.Invoke(CurrentHealth, Profile.MaxHealth);

		return healed;
	}
	#endregion

	#region Downed
	private IEnumerator DownedTimer(float totalSeconds)
	{
		float timeLeft = totalSeconds;
		CurrentDownedTimer = timeLeft;

		while (timeLeft > 0f && State == LifeState.Downed)
		{
			if (reviveCo == null)
			{
				timeLeft -= Time.deltaTime;
				CurrentDownedTimer = timeLeft;
			}

			yield return null;
		}

		if (State == LifeState.Downed)
		{
			Die();
		}
	}
	#endregion

	#region Revive
	public bool CanInteract(Interactor interactor)
	{
		// only downed actors can be revived
		if (State != LifeState.Downed)
		{
			return false;
		}

		if (interactor == null)
		{
			return false;
		}

		// the thing interacting with me should also be a NetActor (the reviver)
		NetActor reviver = interactor.GetComponent<NetActor>();
		if (reviver == null)
		{
			return false;
		}

		// no reviving yourself
		if (reviver == this)
		{
			return false;
		}

		// only allow the local player to start revives
		if (!reviver.IsLocalPlayer)
		{
			return false;
		}

		return true;
	}

	public void OnInteractStart(Interactor interactor)
	{
		if (!CanInteract(interactor))
		{
			return;
		}

		NetActor reviver = interactor.GetComponent<NetActor>();
		if (reviver == null)
		{
			return;
		}

		if (playerController != null)
		{
			playerController.SetCanMove(false);
		}

		reviver.SendRevivePacket(this, ReviveAction.Start);
	}

	public void OnInteractCancel(Interactor interactor)
	{
		NetActor reviver = interactor.GetComponent<NetActor>();
		if (reviver == null)
		{
			return;
		}

		if (playerController != null)
		{
			playerController.SetCanMove(true);
		}

		reviver.SendRevivePacket(this, ReviveAction.Cancel);
	}

	public void StartRevive()
	{
		if (State != LifeState.Downed || reviveCo != null)
		{
			return;
		}

		if (playerProfile == null)
		{
			return;
		}

		reviveCo = StartCoroutine(ReviveRoutine(playerProfile.Revive.ReviveHoldSeconds, playerProfile.Revive.ReviveHealthPercent));
	}

	public void CancelRevive()
	{
		if (reviveCo == null)
		{
			return;
		}

		StopCoroutine(reviveCo);
		reviveCo = null;
	}

	public void SendRevivePacket(NetActor target, ReviveAction action)
	{
		CSteamID targetSteam = target.GetComponent<PlayerSteamID>().SteamID;

		ActorReviveData data = new ActorReviveData
		{
			TargetID = targetSteam.m_SteamID,
			Action = action,
		};
		byte[] payload = PacketManagerASFR.Serialize(PacketID.ActorReviveData, data);

		ReviveOnClientEvent(payload);
		if (!SteamworksManagerASFR.instance.IsHost())
		{
			SteamworksManagerASFR.instance.FireServer("ReviveOnServer", payload);
		}
		else
		{
			SteamworksManagerASFR.instance.FireAllClients("ReviveOnClient", payload);
		}
	}

	private void ReviveOnClientEvent(byte[] payload)
	{
		(PacketID id, IPacketData data) = PacketManagerASFR.Deserialize(payload);
		if (data is ActorReviveData reviveData)
		{
			if (NetGameBootstrapper.PlayerObjects.TryGetValue(new CSteamID(reviveData.TargetID), out GameObject targetGo))
			{
				NetActor targetActor = targetGo.GetComponent<NetActor>();
				switch (reviveData.Action)
				{
					case ReviveAction.Start:
						targetActor.StartRevive();
						break;
					case ReviveAction.Cancel:
						targetActor.CancelRevive();
						break;
				}
			}
		}
	}

	private void ReviveOnServerEvent(CSteamID steamID, byte[] payload)
	{
		ReviveOnClientEvent(payload);

		foreach (CSteamID member in SteamworksManagerASFR.instance.LobbyMembers)
		{
			if (member != steamID && member != SteamworksManagerASFR.instance.LobbyOwner)
			{
				SteamworksManagerASFR.instance.FireClient(member, "ReviveOnClient", payload);
			}
		}
	}

	private IEnumerator ReviveRoutine(float holdSeconds, float healthPercent)
	{
		float t = 0f;
		while (t < holdSeconds && State == LifeState.Downed)
		{
			t += Time.deltaTime;
			CurrentReviveTimer = t;
			yield return null;
		}

		reviveCo = null;

		if (State != LifeState.Downed)
		{
			yield break;
		}

		State = LifeState.Alive;
		if (playerController != null)
		{
			playerController.SetCanMove(true);
		}

		ActorLifeState = State;
		OnLifeStateChanged?.Invoke(State);

		// Equip weapon animation - use instance-based call so RPC syncs to all players
		if (proceduralAnimationController != null)
		{
			proceduralAnimationController.EquipWeapon(instant: false, duration: 6f);
		}

		float restored = Mathf.Max(1, Profile.MaxHealth * healthPercent);
		CurrentHealth = restored;
		ActorHealthAmount = CurrentHealth;
		OnHealthChanged?.Invoke(CurrentHealth, Profile.MaxHealth);
		Debug.Log($"Restored Heralth: {restored}");
		if (animationHandler != null)
		{
			animationHandler.RPC(nameof(animationHandler.SetDowned), false);
		}

		if (downedTimerCo != null)
		{
			StopCoroutine(downedTimerCo);
			downedTimerCo = null;
		}
	}
	#endregion

	#region Die
	private void Die()
	{
		if (State == LifeState.Dead)
		{
			return;
		}

		if (reviveCo != null)
		{
			StopCoroutine(reviveCo);
			reviveCo = null;
		}

		if (downedTimerCo != null)
		{
			StopCoroutine(downedTimerCo);
			downedTimerCo = null;
		}

		State = LifeState.Dead;
		ActorLifeState = State;
		OnLifeStateChanged?.Invoke(State);
		CurrentHealth = 0;
		ActorHealthAmount = CurrentHealth;

		// Deequip weapon animation - use instance-based call so RPC syncs to all players
		if (proceduralAnimationController != null)
		{
			proceduralAnimationController.DeequipWeapon(instant: true);
		}

		SendDiePackage();

		if (MissionManager.instance != null)
		{
			MissionManager.instance.NotifyPlayerDied(this);
		}
	}

	private void SendDiePackage()
	{
		if (playerSteamId == null)
		{
			return;
		}

		CSteamID mySteam = playerSteamId.SteamID;

		ActorDieData data = new ActorDieData
		{
			TargetID = mySteam.m_SteamID
		};

		byte[] payload = PacketManagerASFR.Serialize(PacketID.ActorDieData, data);

		ActorDiedOnClientEvent(payload);
		if (!SteamworksManagerASFR.instance.IsHost())
		{
			SteamworksManagerASFR.instance.FireServer("ActorDiedOnServer", payload);
		}
		else
		{
			SteamworksManagerASFR.instance.FireAllClients("ActorDiedOnClient", payload);
		}
	}

	private void ActorDiedOnClientEvent(byte[] payload)
	{
		(PacketID id, IPacketData data) = PacketManagerASFR.Deserialize(payload);
		if (data is ActorDieData dieData)
		{
			if (NetGameBootstrapper.PlayerObjects.TryGetValue(new CSteamID(dieData.TargetID), out GameObject targetGo))
			{
				NetActor actor = targetGo.GetComponent<NetActor>();

				if (actor.State != LifeState.Dead)
				{
					actor.Die();
				}
			}
		}
	}

	private void ActorDiedOnServerEvent(CSteamID sender, byte[] payload)
	{
		ActorDiedOnClientEvent(payload);

		foreach (CSteamID member in SteamworksManagerASFR.instance.LobbyMembers)
		{
			if (member != sender && member != SteamworksManagerASFR.instance.LobbyOwner)
			{
				SteamworksManagerASFR.instance.FireClient(member, "ActorDiedOnClient", payload);
			}
		}
	}
	#endregion

	#region Ready
	public void SetReady(bool ready)
	{
		if (Ready == ready)
		{
			return;
		}

		SendReadyPacket(ready);
	}

	public void SetReadyLocal(bool ready)
	{
		Ready = ready;
		IsReady = ready;
	}

	private void SendReadyPacket(bool ready)
	{
		if (playerSteamId == null)
		{
			return;
		}

		CSteamID mySteam = playerSteamId.SteamID;

		ActorReadyData data = new ActorReadyData
		{
			PlayerID = mySteam.m_SteamID,
			Ready = ready
		};

		byte[] payload = PacketManagerASFR.Serialize(PacketID.ActorReadyData, data);

		ActorReadyOnClientEvent(payload);

		if (!SteamworksManagerASFR.instance.IsHost())
		{
			SteamworksManagerASFR.instance.FireServer("ActorReadyOnServer", payload);
		}
		else
		{
			SteamworksManagerASFR.instance.FireAllClients("ActorReadyOnClient", payload);
		}
	}

	private void ActorReadyOnClientEvent(byte[] payload)
	{
		(PacketID id, IPacketData data) = PacketManagerASFR.Deserialize(payload);
		if (data is ActorReadyData readyData)
		{
			if (NetGameBootstrapper.PlayerObjects.TryGetValue(new CSteamID(readyData.PlayerID), out GameObject targetGo))
			{
				NetActor actor = targetGo.GetComponent<NetActor>();

				actor.SetReadyLocal(readyData.Ready);
				CheckIfAllReady();
			}
		}
	}

	private void ActorReadyOnServerEvent(CSteamID sender, byte[] payload)
	{
		ActorReadyOnClientEvent(payload);

		foreach (CSteamID member in SteamworksManagerASFR.instance.LobbyMembers)
		{
			if (member != sender && member != SteamworksManagerASFR.instance.LobbyOwner)
			{
				SteamworksManagerASFR.instance.FireClient(member, "ActorReadyOnClient", payload);
			}
		}
	}

	private void CheckIfAllReady()
	{
		int total = NetGameBootstrapper.PlayerObjects.Count;
		bool allReady = true;
		int ready = 0;

		foreach (var playerEntry in NetGameBootstrapper.PlayerObjects)
		{
			GameObject playerGo = playerEntry.Value;
			if (playerGo == null)
			{
				continue;
			}

			NetActor actor = playerGo.GetComponent<NetActor>();
			if (actor == null)
			{
				continue;
			}

			if (!actor.Ready)
			{
				allReady = false;
			}
			else
			{
				ready++;
			}
		}

		PlayerIndicatorVisualUpdater.instance.UpdateReadyIndicator($"{ready} / {total} players are ready");

		if (allReady)
		{
			Debug.Log("[NetActor] All players ready – starting countdown.");
			SendMatchTimerPacket(MatchTimerAction.Start, matchTimerDuration);
		}
		else
		{
			Debug.Log("[NetActor] Not all players ready – cancelling countdown.");
			SendMatchTimerPacket(MatchTimerAction.Cancel);
		}
	}

	public void StartMatchTimer(float duration)
	{
		if (matchTimerCo != null)
		{
			return;
		}

		if (MatchmakingSystem.instance.GetSelectedLevelID() != -1)
		{
			matchTimerCo = StartCoroutine(MatchTimerRoutine(duration));
		}
		else
		{
			PlayerIndicatorVisualUpdater.instance.UpdateReadyIndicator("No mission selected\nGo back to the Terminal to select one.");
		}
	}

	public void CancelMatchTimer()
	{
		if (matchTimerCo == null)
		{
			return;
		}

		StopCoroutine(matchTimerCo);
		matchTimerCo = null;

		matchTimerRunning = false;
		MatchTimerRemaining = 0f;
	}

	private void UpdateMatchTimerText(float matchTimerRemaining)
	{
		int minutes = (int)(matchTimerRemaining / 60f);
		int seconds = (int)(matchTimerRemaining % 60f);

		string formatted = $"{minutes:00}:{seconds:00}";
		PlayerIndicatorVisualUpdater.instance.UpdateReadyIndicator($"Game starts in\n{formatted}");
	}

	private IEnumerator MatchTimerRoutine(float duration)
	{
		matchTimerDuration = duration;
		MatchTimerRemaining = duration;
		matchTimerRunning = true;

		while (MatchTimerRemaining > 0f)
		{
			MatchTimerRemaining -= Time.deltaTime;
			if (MatchTimerRemaining < 0f)
			{
				MatchTimerRemaining = 0f;
			}
			UpdateMatchTimerText(MatchTimerRemaining);

			yield return null;
		}

		matchTimerRunning = false;
		matchTimerCo = null;

		Debug.Log("[NetActor] Match timer finished -> start game here!");
		MatchmakingSystem.instance.StartSelectedLevel();
	}

	public void SendMatchTimerPacket(MatchTimerAction action, float duration = 0f)
	{
		MatchTimerData data = new MatchTimerData
		{
			Duration = duration,
			Action = action
		};

		byte[] payload = PacketManagerASFR.Serialize(PacketID.MatchTimerData, data);

		MatchTimerOnClientEvent(payload);

		if (!SteamworksManagerASFR.instance.IsHost())
		{
			SteamworksManagerASFR.instance.FireServer("MatchTimerOnServer", payload);
		}
		else
		{
			SteamworksManagerASFR.instance.FireAllClients("MatchTimerOnClient", payload);
		}
	}

	private void MatchTimerOnClientEvent(byte[] payload)
	{
		(PacketID id, IPacketData baseData) = PacketManagerASFR.Deserialize(payload);
		if (baseData is MatchTimerData timerData)
		{
			switch (timerData.Action)
			{
				case MatchTimerAction.Start:
					StartMatchTimer(timerData.Duration);
					break;
				case MatchTimerAction.Cancel:
					CancelMatchTimer();
					break;
			}
		}
	}

	private void MatchTimerOnServerEvent(CSteamID sender, byte[] payload)
	{
		MatchTimerOnClientEvent(payload);

		foreach (CSteamID member in SteamworksManagerASFR.instance.LobbyMembers)
		{
			if (member != sender && member != SteamworksManagerASFR.instance.LobbyOwner)
			{
				SteamworksManagerASFR.instance.FireClient(member, "MatchTimerOnClient", payload);
			}
		}
	}
	#endregion
}
