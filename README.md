# Slacky
Slack の Web API を使ってチャネルへメッセージを投稿する Windows 向けの軽量 CLI ツールです。

投稿完了時に Slack Bot のアイコンと名前を使った Windows トースト通知が表示されます。


## ビルド方法
Visual Studio 2022 でビルドできます。

テストプロジェクトの Google Test 以外に依存する外部ライブラリはありません。


## 使い方
Slack Bot の OAuth Token が必要です。

```console
slacky.exe <token> <channel> <message>
```

- token ... Bot User OAuth Token（例：xoxb-...）
- channel ... 投稿先のチャネル ID（例：Cxxxxxx）
- message ... メッセージ本文

token は環境変数 SLACKY_TOKEN に設定しておくこともできます。その場合はコマンドライン引数から省略可能です。
message は、メッセージ本文の代わりにメッセージを書いたテキストファイルの名前を指定することもできます。ファイル名の先頭に @ を付けて指定します。

```console
slacky.exe <channel> @message.txt
```


## チャネル ID の取得方法
チャネル ID は「チャネルの詳細」から確認できます。チャネルを表示した時の URL の末尾と同じです。


## OAuth Token の取得方法
1. [Your Apps](https://api.slack.com/apps) にアクセスし Slack ワークスペースにアプリを作成します。
アプリの作成方法は「From scratch」を選択します。アプリの名前とワークスペースを指定するだけです。
1. アプリの作成後、アプリの「Basic Information」ページに遷移します。
ここでは「Display Information」セクションで Bot アイコンと名前を設定します。他の設定は特に要りません。
1. 左側のメニューから「Install App」を選択します。
先に「Scopes」セクションで「chat:write」権限を追加します。すると「Install to Workspace」ボタンが有効化されるのでクリックします。
1. 「Bot User OAuth Token」が発行されるのでコピーします。
