# -*- coding: utf-8 -*-
"""Regenerate *.po from omnishell.pot with UTF-8 translations. Run: python3 po/regen_po.py"""
from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parent

T = {
    "Module is not enabled": ("模块未启用", "模組未啟用", "모듈이 사용 설정되어 있지 않습니다", "モジュールが有効になっていません"),
    "Error": ("错误", "錯誤", "오류", "エラー"),
    "Failed to launch %s: %s": ("无法启动 %s：%s", "無法啟動 %s：%s", "%s을(를) 시작하지 못했습니다: %s", "%s を起動できません: %s"),
    "Search programs...": ("搜索程序…", "搜尋程式…", "프로그램 검색...", "プログラムを検索..."),
    "All programs": ("所有程序", "所有程式", "모든 프로그램", "すべてのプログラム"),
    "  (empty)": ("  （空）", "  （空白）", "  (비어 있음)", "  （空）"),
    "  (no recent items)": ("  （无最近项）", "  （無最近項目）", "  (최근 항목 없음)", "  （最近使った項目はありません）"),
    "Recent files": ("最近使用的文件", "最近的檔案", "최근 파일", "最近使ったファイル"),
    "Trash": ("回收站", "資源回收筒", "휴지통", "ごみ箱"),
    "Exit": ("退出", "結束", "종료", "終了"),
    "Start": ("开始", "開始", "시작", "スタート"),
    "Categories:": ("类别：", "類別：", "범주:", "カテゴリ:"),
    "Module Manager": ("模块管理器", "模組管理員", "모듈 관리자", "モジュール マネージャー"),
    "Manage installed modules": ("管理已安装的模块", "管理已安裝的模組", "설치된 모듈 관리", "インストール済みモジュールの管理"),
    "System Settings": ("系统设置", "系統設定", "시스템 설정", "システム設定"),
    "Configure system options": ("配置系统选项", "設定系統選項", "시스템 옵션 구성", "システムオプションの設定"),
    "User Configuration": ("用户配置", "使用者設定", "사용자 구성", "ユーザー設定"),
    "User preferences": ("用户首选项", "使用者喜好設定", "사용자 기본 설정", "ユーザー設定"),
    "Desktop": ("桌面", "桌面", "바탕 화면", "デスクトップ"),
    "Desktop appearance": ("桌面外观", "桌面外觀", "바탕 화면 모양", "デスクトップの外観"),
    "Display": ("显示", "顯示", "디스플레이", "ディスプレイ"),
    "Screen settings": ("屏幕设置", "螢幕設定", "화면 설정", "画面設定"),
    "About": ("关于", "關於", "정보", "バージョン情報"),
    "System information": ("系统信息", "系統資訊", "시스템 정보", "システム情報"),
    "Select a category to view options": ("选择一个类别以查看选项", "選擇類別以檢視選項", "옵션을 보려면 범주를 선택하세요", "オプションを表示するにはカテゴリを選択してください"),
    "Installed Modules:": ("已安装模块：", "已安裝模組：", "설치된 모듈:", "インストール済みモジュール:"),
    "Name": ("名称", "名稱", "이름", "名前"),
    "Description": ("描述", "描述", "설명", "説明"),
    "Status": ("状态", "狀態", "상태", "状態"),
    "Enabled": ("已启用", "已啟用", "사용", "有効"),
    "Disabled": ("已禁用", "已停用", "사용 안 함", "無効"),
    "Enable": ("启用", "啟用", "사용", "有効にする"),
    "Disable": ("禁用", "停用", "사용 안 함", "無効にする"),
    "Close": ("关闭", "關閉", "닫기", "閉じる"),
    "System": ("系统", "系統", "시스템", "システム"),
    "OS Name: %s": ("操作系统名称：%s", "作業系統名稱：%s", "OS 이름: %s", "OS 名: %s"),
    "OS Version: %s": ("操作系统版本：%s", "作業系統版本：%s", "OS 버전: %s", "OS バージョン: %s"),
    "User": ("用户", "使用者", "사용자", "ユーザー"),
    "Name: %s": ("名称：%s", "名稱：%s", "이름: %s", "名前: %s"),
    "Role: %s": ("角色：%s", "角色：%s", "역할: %s", "役割: %s"),
    "Background Settings": ("背景设置", "背景設定", "배경 설정", "背景の設定"),
    "Arrange Desktop Icons": ("排列桌面图标", "排列桌面圖示", "바탕 화면 아이콘 정렬", "デスクトップアイコンの整列"),
    "Resolution: %d x %d": ("分辨率：%d × %d", "解析度：%d × %d", "해상도: %d x %d", "解像度: %d × %d"),
    "About OmniShell": ("关于 OmniShell", "關於 OmniShell", "OmniShell 정보", "OmniShell について"),
    "OmniShell Desktop Environment": ("OmniShell 桌面环境", "OmniShell 桌面環境", "OmniShell 데스크톱 환경", "OmniShell デスクトップ環境"),
    "Version: 1.1.1": ("版本：1.1.1", "版本：1.1.1", "버전: 1.1.1", "バージョン: 1.1.1"),
    "wxMediaCtrl is not available (link wxWidgets media library).": (
        "wxMediaCtrl 不可用（请链接 wxWidgets 媒体库）。",
        "wxMediaCtrl 無法使用（請連結 wxWidgets 媒體程式庫）。",
        "wxMediaCtrl을(를) 사용할 수 없습니다(wxWidgets 미디어 라이브러리를 연결하세요).",
        "wxMediaCtrl が利用できません（wxWidgets メディア ライブラリをリンクしてください）。",
    ),
    "Play": ("播放", "播放", "재생", "再生"),
    "Pause": ("暂停", "暫停", "일시 정지", "一時停止"),
    "Stop": ("停止", "停止", "중지", "停止"),
    "Open media": ("打开媒体", "開啟媒體", "미디어 열기", "メディアを開く"),
    "Audio/Video": ("音频/视频", "音訊/視訊", "오디오/비디오", "オーディオ/ビデオ"),
    "All files": ("所有文件", "所有檔案", "모든 파일", "すべてのファイル"),
    "Unknown exception in %s": ("未知异常：%s", "未知的例外狀況：%s", "알 수 없는 예외(%s)", "不明な例外 (%s)"),
    "User name:": ("用户名：", "使用者名稱：", "사용자 이름:", "ユーザー名:"),
    "Password:": ("密码：", "密碼：", "암호:", "パスワード:"),
    "Sign in": ("登录", "登入", "로그인", "サインイン"),
}

MULTI = (
    "未处理的异常：%s\n\n%s",
    "未處理的例外狀況：%s\n\n%s",
    "처리되지 않은 예외(%s)\n\n%s",
    "未処理の例外 (%s)\n\n%s",
)


def esc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main() -> None:
    pot = (ROOT / "omnishell.pot").read_text(encoding="utf-8")
    langs = [("zh_CN", 0), ("zh_TW", 1), ("ko_KR", 2), ("ja_JP", 3)]
    for code, li in langs:
        body = pot
        body = body.replace("SOME DESCRIPTIVE TITLE.", f"{code} translations for omnishell.")
        body = body.replace("CHARSET", "UTF-8")
        body = body.replace('"Language: \\n"', f'"Language: {code}\\n"')
        body = body.replace('"Project-Id-Version: PACKAGE VERSION\\n"', '"Project-Id-Version: omnishell\\n"')
        body = body.replace(
            '"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"', '"PO-Revision-Date: 2026-04-07 00:00+0000\\n"'
        )
        body = body.replace(
            '"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"', '"Last-Translator: OmniShell <>\\n"'
        )
        body = body.replace('"Language-Team: LANGUAGE <LL@li.org>\\n"', f'"Language-Team: {code} <>\\n"')
        body = body.replace("#, fuzzy\n", "", 1)
        for k, tup in T.items():
            old = f'msgid "{k}"\nmsgstr ""\n'
            new = f'msgid "{k}"\nmsgstr "{esc(tup[li])}"\n'
            if old not in body:
                raise SystemExit(f"missing: {k!r}")
            body = body.replace(old, new, 1)
        oldm = (
            '#, c-format\nmsgid ""\n'
            '"Unhandled exception in %s\\n"\n'
            '"\\n"\n'
            '"%s"\n'
            'msgstr ""\n'
        )
        p0, p1 = MULTI[li].split("\n\n", 1)
        newm = (
            '#, c-format\nmsgid ""\n'
            '"Unhandled exception in %s\\n"\n'
            '"\\n"\n'
            '"%s"\n'
            'msgstr ""\n'
            f'"{esc(p0)}\\n"\n'
            '"\\n"\n'
            f'"{esc(p1)}"\n'
        )
        if oldm not in body:
            raise SystemExit("missing multiline block")
        body = body.replace(oldm, newm, 1)
        (ROOT / f"{code}.po").write_text(body, encoding="utf-8")
    print("wrote", ", ".join(f"{c}.po" for c, _ in langs))


if __name__ == "__main__":
    main()
