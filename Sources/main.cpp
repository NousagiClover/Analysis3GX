#include <3ds.h>
#include "csvc.h"
#include <CTRPluginFramework.hpp>

#include <vector>
#include "cheats.hpp"

namespace CTRPluginFramework
{
    // This patch the NFC disabling the touchscreen when scanning an amiibo, which prevents ctrpf to be used
    static void    ToggleTouchscreenForceOn(void)
    {
        static u32 original = 0;
        static u32 *patchAddress = nullptr;

        if (patchAddress && original)
        {
            *patchAddress = original;
            return;
        }

        static const std::vector<u32> pattern =
        {
            0xE59F10C0, 0xE5840004, 0xE5841000, 0xE5DD0000,
            0xE5C40008, 0xE28DD03C, 0xE8BD80F0, 0xE5D51001,
            0xE1D400D4, 0xE3510003, 0x159F0034, 0x1A000003
        };

        Result  res;
        Handle  processHandle;
        s64     textTotalSize = 0;
        s64     startAddress = 0;
        u32 *   found;

        if (R_FAILED(svcOpenProcess(&processHandle, 16)))
            return;

        svcGetProcessInfo(&textTotalSize, processHandle, 0x10002);
        svcGetProcessInfo(&startAddress, processHandle, 0x10005);
        if(R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, processHandle, (u32)startAddress, textTotalSize)))
            goto exit;

        found = (u32 *)Utils::Search<u32>(0x14000000, (u32)textTotalSize, pattern);

        if (found != nullptr)
        {
            original = found[13];
            patchAddress = (u32 *)PA_FROM_VA((found + 13));
            found[13] = 0xE1A00000;
        }

        svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, textTotalSize);
exit:
        svcCloseHandle(processHandle);
    }

    static MenuEntry* EntryWithHotkey(MenuEntry* entry, const Hotkey& hotkey) {
        if (entry != nullptr) {
            entry->Hotkeys += hotkey;
            entry->SetArg(new std::string(entry->Name()));
            entry->Name() += " " + hotkey.ToString();
            entry->Hotkeys.OnHotkeyChangeCallback([](MenuEntry* entry, int index) {
                auto* name = reinterpret_cast<std::string*>(entry->GetArg());
                entry->Name() = *name + " " + entry->Hotkeys[0].ToString();
            });
        }
        return entry;
    }

    static MenuEntry* EntryWithHotkey(MenuEntry* entry,
                                    const std::vector<Hotkey>& hotkeys) {
        for (const Hotkey& kHotkey : hotkeys) {
            entry->Hotkeys += kHotkey;
        }
        return entry;
    }

    static MenuEntry* EnableEntry(MenuEntry* entry) {
        if (entry != nullptr) {
            entry->SetArg(new std::string(entry->Name()));
            entry->Enable();
        }
        return entry;
    }
    
    // This function is called before main and before the game starts
    // Useful to do code edits safely
    void    PatchProcess(FwkSettings &settings)
    {
        ToggleTouchscreenForceOn();
    }

    // This function is called when the process exits
    // Useful to save settings, undo patchs or clean up things
    void    OnProcessExit(void)
    {
        ToggleTouchscreenForceOn();
    }

    void    InitMenu(PluginMenu &menu)
    {
        // notes
        std::string noteAutoPatchCode =
        "開始アドレスと終了アドレスを設定してください。\n"
        "開始アドレスと終了アドレスが同じ場合は、アドレスの初期化をしてください。\n\n"
        "次に [ファイルに書き込み] を押下してください。\n\n"
        "/lumna/plugins/" + getFilePath() + " にコードが作成されます。\n\n" +
        (Color::Red << "※注意\n") +
        "CTRPF上でコードが作成されたファイルを開くと、\n"
        "新しくコードが作成できません。その場合はゲームを再起動してからコードを作成してください。\n\n" +
        (Color::White << "Enjoy coding!");

        std::string noteMaxMinValue =
        "指定したアドレスの値の最大値と最小値を調査します。\n"
        "現在の値と前回の値を比較して、前回よりも更新すれば\n"
        "最大小値が更新されます。\n"
        "また、値の履歴を見ることができます。";

        std::string noteRegConf =
        "ROMアドレスにレジスタ確認パッチを適応します。\n"
        "レジスタ確認パッチコードは\n"
        "0x1E81000 ~ 0x1E8107C に作成されます。\n"
        "0x1E81000 ~ 0x1E8107C には何も書き込まないでください。";

        std::string noteAddressConf =
        "指定したアドレスの値をリアルタイムで表示します。\n"
        "同時に５つのアドレスを監視することができます。";

        std::string noteRangeWrite =
        "任意のアドレスからアドレスの範囲で指定した値を\n"
        "任意の値で置き換えます。\n"
        "戻るを押すと置き換えた値を元に戻します。\n";

        menu += new MenuEntry(Color::Yellow << "パッチコード作成自動化", nullptr, autoPatchCode, noteAutoPatchCode);
        menu += EntryWithHotkey(new MenuEntry(Color::Orange << "ウォッチポイント（逆アセ）", nullptr, doWPManager), {Hotkey(Key::R | Y, "監視開始"), Hotkey(Key::R | X, "監視終了"), Hotkey(Key::R | A, "一時停止")});
        menu += new MenuEntry(Color::Turquoise << "レジスタ確認", nullptr, regConf, noteRegConf);
        menu += new MenuEntry(Color::Magenta << "アドレス監視", nullptr, addressConf, noteAddressConf);
        menu += new MenuEntry(Color::Lime << "16進数電卓", nullptr, calcHex);
        menu += new MenuEntry(Color::Silver << "分岐値計算", nullptr, calcBranch);
        menu += new MenuEntry(Color::SkyBlue << "最大最小値調査", nullptr, maxMinValue, noteMaxMinValue);
        menu += new MenuEntry(Color::LimeGreen << "範囲書き込み", nullptr, rangeWrite, noteRangeWrite);
        menu += new MenuEntry(Color::Cyan << "空きメモリ確保", nullptr, getFreeMemory);
    }

    int     main(void)
    {
        PluginMenu *menu = new PluginMenu(Color::Red << "Analysis CTRPF 3GX", 1, 0, 2,
                                            "元となった3gx作成者: xv\n"
                                            "Twitter: xvcfw_\n"
                                            "YouTube: xvcfw\n"
                                            "Discord: xvcfw\n"
                                            "--------------------------------\n"
                                            "拡張版3gx作成者: Clover\n"
                                            "Discord: holo_clover"
                                            );

        // Synnchronize the menu with frame event
        menu->SynchronizeWithFrame(true);
        menu->ShowWelcomeMessage(false);
        if (! File::Exists("./CTRPFData.bin"))
        {
            MessageBox("Analysis CTRPF 3GX",
                "この3gxは解析専用の3gxです。\n"
                "xv様の逆アセ3gxをベースに作りました。\n"
                "逆アセ3gx開発者のxv様に感謝して使いましょう。\n"
                "------------------------------------------\n"
                "xv様\n"
                "Twitter: @xvcfw_\n"
                "YouTube: xvcfw\n"
                "Discord: xvcfw")();
            Sleep(Milliseconds(100));
            MessageBox("Analysis CTRPF 3GX", "それでは良い解析を！\n※以降はメッセージ表示されません")();
        }
        OSD::Notify("Hello, world!");
        OSD::Notify("3GX for game analysis!");
        OSD::Notify("Version: 1.0.2");
        OSD::Notify(Utils::Format("TitleID: 0004000000%06X", Process::GetTitleID()));

        InitMenu(*menu);
        menu->Run();
        delete menu;

        // Exit plugin
        return (0);
    }
}
