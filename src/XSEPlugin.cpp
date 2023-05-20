#include <SimpleIni.h>

bool freeLeft = false;
bool freeRight = true;
bool freeCombat = false;

void loadIni()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\GamepadButtonFreer.ini");

	freeLeft = ini.GetBoolValue("settings", "bFreeLeftHand", false);
	freeRight = ini.GetBoolValue("settings", "bFreeRightHand", true);
	freeCombat = ini.GetBoolValue("settings", "bFreeInCombat", false);
}

struct Hooks
{
	static void Hook()
	{
		REL::Relocation<std::uintptr_t> AttackBlockHandlerVtbl{ RE::VTABLE_AttackBlockHandler[0] };
		_ProcessAttackBlock = AttackBlockHandlerVtbl.write_vfunc(0x4, ProcessAttackBlock);
	}

	static void ProcessAttackBlock(RE::AttackBlockHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto userEvent = RE::UserEvents::GetSingleton();

		if (player->AsActorState()->IsWeaponDrawn() || player->IsInCombat() && !freeCombat || a_event->userEvent == userEvent->leftAttack && !freeLeft || a_event->userEvent == userEvent->rightAttack && !freeRight)
			return _ProcessAttackBlock(a_this, a_event, a_data);
	}
	static inline REL::Relocation<decltype(ProcessAttackBlock)> _ProcessAttackBlock;
};

void Init()
{
	loadIni();
	Hooks::Hook();
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format("{}.log"sv, Plugin::NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();

	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse();
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
