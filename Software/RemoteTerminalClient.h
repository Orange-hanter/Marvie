#pragma once

#include <QIODevice>
#include <QTextEdit>
#include <QMenu>

class RemoteTerminalClient : public QTextEdit
{
	Q_OBJECT;

public:
	RemoteTerminalClient( QWidget* parent = nullptr );
	RemoteTerminalClient( const RemoteTerminalClient& ) = delete;
	RemoteTerminalClient( RemoteTerminalClient&& ) = delete;
	~RemoteTerminalClient();

	void setIODevice( QIODevice* device );
	void synchronize();
	bool isSynchronize();
	void disable();

private:
	void keyPressEvent( QKeyEvent* e ) override;
	void mousePressEvent( QMouseEvent* e ) override;
	void mouseMoveEvent( QMouseEvent* e ) override;
	void mouseReleaseEvent( QMouseEvent* e ) override;
	void mouseDoubleClickEvent( QMouseEvent* e ) override;

	void contextMenuEvent( QContextMenuEvent* e ) override;

	void dragEnterEvent( QDragEnterEvent* e ) override;
	void dragLeaveEvent( QDragLeaveEvent* e ) override;
	void dragMoveEvent( QDragMoveEvent* e ) override;
	void dropEvent( QDropEvent* e ) override;

	int keyCode( Qt::Key key );
	void moveText( QString& str );

private slots:
	void processBytes();
	void aboutToClose();

private:
	QIODevice* device;
	QString linePrefix;
	int limitPosition;
	QList< QString > history;
	int historyIndex;
	QString errMessage;
	bool replaceMode;
	enum class State
	{
		Disabled,
		Sync,
		WaitPrefix,
		Input,
		WaitResponse,
		WaitErrorMessage,
		WaitNotFoundCommandName,
		Exec,
		WaitMoveN,
		WaitTextColor,
		WaitBackgroundColor,
		WaitTermination,
	} state, nextState;

	enum Control : uint8_t
	{
		Sync = 255,
		Prefix = 1,
		CommandNotFound,
		Ok,
		Error,
		ProgramStarted,
		TerminateRequest,
		ProgramFinished,
		ProgramFinishedAck,

		CursorReplaceModeEnable,
		CursorReplaceModeDisable,
		CursorMoveLeft,
		CursorMoveRight,
		CursorMoveN,
		CursorMoveToEnd,
		CursorMoveToBegin,
		CursorMoveToEndOfLine,
		CursorMoveToBeginOfLine,

		ConsoleDelete,
		ConsoleBackspace,
		ConsoleClear,
		ConsoleSetTextColor,
		ConsoleSetBackgroundColor,
		ConsoleRestoreColors,
		ConsoleCommit,

		KeyEscape = 100,
		KeyTab,
		KeyBacktab,
		KeyBackspace,
		KeyReturn,
		KeyEnter,
		KeyInsert,
		KeyDelete,
		KeyPause,
		KeyPrint,
		KeySysReq,
		KeyClear,
		KeyHome,
		KeyEnd,
		KeyLeft,
		KeyUp,
		KeyRight,
		KeyDown,
		KeyPageUp,
		KeyPageDown,
		KeyShift,
		KeyControl,
		KeyMeta,
		KeyAlt,
		KeyCapsLock,
		KeyNumLock,
		KeyScrollLock,
		KeyF1,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,
		Char255
	};
};