#include "RemoteTerminalClient.h"
#include <QTextDocumentFragment>
#include <QGuiApplication>
#include <QClipboard>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

RemoteTerminalClient::RemoteTerminalClient( QWidget* parent /*= nullptr */ ) : QTextEdit( parent )
{
	device = nullptr;
	limitPosition = 0;
	historyIndex = 0;
	replaceMode = false;
	state = State::Disabled;
	nextState = State::Disabled;

	QFont f( "Droid Sans Mono" );
	f.setPointSize( 10 );
	setFont( f );
	setCursorWidth( QFontMetrics( font() ).width( QChar( 'X' ) ) );
	setUndoRedoEnabled( false );
}

RemoteTerminalClient::~RemoteTerminalClient()
{

}

void RemoteTerminalClient::setIODevice( QIODevice* device )
{
	disable();
	if( this->device )
	{
		QObject::disconnect( this->device, &QIODevice::readyRead, this, &RemoteTerminalClient::processBytes );
		QObject::disconnect( this->device, &QIODevice::aboutToClose, this, &RemoteTerminalClient::aboutToClose );
	}

	this->device = device;
	if( device )
	{
		QObject::connect( device, &QIODevice::readyRead, this, &RemoteTerminalClient::processBytes );
		QObject::connect( device, &QIODevice::aboutToClose, this, &RemoteTerminalClient::aboutToClose );
	}
}

void RemoteTerminalClient::synchronize()
{
	if( device == nullptr || !device->isOpen() )
		return;

	clear();
	setReadOnly( true );
	linePrefix.clear();
	limitPosition = -1;
	uint8_t data[2] = { 255, Control::Sync };
	state = State::Sync;
	device->write( ( const char* )data, 2 );
}

bool RemoteTerminalClient::isSynchronize()
{
	return state != State::Sync && state != State::Disabled;
}

void RemoteTerminalClient::disable()
{
	setReadOnly( true );
	state = State::Disabled;
}

void RemoteTerminalClient::keyPressEvent( QKeyEvent* e )
{
	//qDebug() << e->key() << " " << e->nativeScanCode() << e->nativeVirtualKey();
	if( state == State::Exec )
	{
		if( e->key() < 255 )
		{
			char c = ( char )e->key();
			if( e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z )
			{
				if( e->key() == Qt::Key_Z && e->modifiers() == Qt::KeyboardModifier::ControlModifier )
				{
					state = State::WaitTermination;
					uint8_t data[2] = { 255, Control::TerminateRequest };
					device->write( ( const char* )data, 2 );
					return;
				}
				if( e->modifiers() == Qt::KeyboardModifier::ShiftModifier )
					c = 'A' + ( char )( e->key() - Qt::Key_A );
				else
					c = 'a' + ( char )( e->key() - Qt::Key_A );
			}
			device->write( &c, 1 );
		}
		else if( e->key() == 255 )
		{
			uint8_t data[2] = { 255, Control::Char255 };
			device->write( ( const char* )data, 2 );
		}
		else
		{
			int code = keyCode( ( Qt::Key )e->key() );
			if( code == -1 )
				return;
			uint8_t data[2] = { 255, ( uint8_t )code };
			device->write( ( const char* )data, 2 );
		}
		return;
	}
	else if( state != State::Input )
		return;

	auto cur = textCursor();
	int min = qMin( cur.anchor(), cur.position() );
	int max = qMax( cur.anchor(), cur.position() );
	/*int minPos = qMax( min, limitPosition );
	int maxPos = qMax( max, limitPosition );*/

	if( e->key() == Qt::Key_Left )
	{
		if( min <= limitPosition )
		{
			cur.setPosition( limitPosition );
			setTextCursor( cur );
			return;
		}
	}
	else if( e->key() == Qt::Key_Right )
	{
		if( max < limitPosition )
		{
			cur.setPosition( limitPosition );
			setTextCursor( cur );
			return;
		}
	}
	else if( min < limitPosition )
		return;

	switch( e->key() )
	{
	case Qt::Key_Backspace:
		if( min <= limitPosition )
			return;
		break;
	case Qt::Key_Delete:
		if( min < limitPosition )
			return;
		break;
	case Qt::Key_Home:
	{
		cur.setPosition( limitPosition );
		setTextCursor( cur );
		return;
	}
	case Qt::Key_Up:
	case Qt::Key_Down:
	{
		if( history.isEmpty() )
			return;
		historyIndex += e->key() == Qt::Key_Up ? 1 : -1;
		if( historyIndex < 0 )
			historyIndex = 0;
		else if( historyIndex >= history.size() )
			historyIndex = history.size() - 1;
		auto line = history[historyIndex];
		auto cur = textCursor();
		cur.setPosition( limitPosition );
		cur.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
		cur.insertText( line );
		return;
	}
	case Qt::Key_PageUp:
	case Qt::Key_PageDown:
		return;
	case Qt::Key_Enter:
	case Qt::Key_Return:
	{
		if( state == State::Input )
		{
			auto cur = textCursor();
			cur.setPosition( limitPosition );
			cur.movePosition( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );
			auto line = cur.selectedText();
			line.remove( QRegExp( "^ +" ) );
			if( !line.isEmpty() )
			{
				history.removeOne( line );
				history.push_front( line );
				if( history.size() > 20 )
					history.pop_back();
			}
			historyIndex = -1;
			moveCursor( QTextCursor::MoveOperation::EndOfLine );
			setReadOnly( true );
			state = State::WaitResponse;
			line.append( '\n' );
			device->write( line.remove( QChar( 255 ) ).toUtf8() );
			return;
		}
		break;
	}
	/*case Qt::Key_A:
		if( e->modifiers() == Qt::KeyboardModifier::ControlModifier )
		{
			auto cur = limit;
			cur.movePosition( QTextCursor::Right );
			cur.movePosition( QTextCursor::End, QTextCursor::KeepAnchor );
			setTextCursor( cur );
			return;
		}
		break;*/
	}

	QTextEdit::keyPressEvent( e );
}

void RemoteTerminalClient::mousePressEvent( QMouseEvent* e )
{
	if( state == State::Input || state == State::Disabled )
		QTextEdit::mousePressEvent( e );
}

void RemoteTerminalClient::mouseMoveEvent( QMouseEvent* e )
{
	if( state == State::Input || state == State::Disabled )
		QTextEdit::mouseMoveEvent( e );
}

void RemoteTerminalClient::mouseReleaseEvent( QMouseEvent* e )
{
	if( state == State::Input || state == State::Disabled )
		QTextEdit::mouseReleaseEvent( e );
}

void RemoteTerminalClient::mouseDoubleClickEvent( QMouseEvent* e )
{
	if( state == State::Input || state == State::Disabled )
		QTextEdit::mouseDoubleClickEvent( e );
}

void RemoteTerminalClient::contextMenuEvent( QContextMenuEvent* e )
{
	QMenu menu;
	menu.addAction( "Cut" )->setEnabled( state == State::Input && textCursor().selectionStart() >= limitPosition );
	menu.addAction( "Copy" )->setEnabled( state == State::Input || state == State::Disabled );
	menu.addAction( "Paste" )->setEnabled( state == State::Input && textCursor().selectionStart() >= limitPosition 
											|| state == State::Exec || state == State::WaitBackgroundColor 
											|| state == State::WaitMoveN || state == State::WaitTermination || state == State::WaitTextColor );
	QAction* action = menu.exec( e->globalPos() );
	if( action )
	{
		if( action->text() == "Cut" )
		{
			if( state == State::Input && textCursor().selectionStart() >= limitPosition )
			{
				QGuiApplication::clipboard()->setText( textCursor().selection().toPlainText() );
				textCursor().removeSelectedText();
			}
		}
		else if( action->text() == "Copy" )
		{
			if( state == State::Input || state == State::Disabled )
				QGuiApplication::clipboard()->setText( textCursor().selection().toPlainText() );
		}
		else if( action->text() == "Paste" )
		{
			if( state == State::Input && textCursor().selectionStart() >= limitPosition
				|| state == State::Exec || state == State::WaitBackgroundColor
				|| state == State::WaitMoveN || state == State::WaitTermination || state == State::WaitTextColor )
				textCursor().insertText( QGuiApplication::clipboard()->text() );
		}
	}
}

void RemoteTerminalClient::dragEnterEvent( QDragEnterEvent* e )
{
	/*qDebug() << "1";
	QTextEdit::dragEnterEvent( e );*/
}

void RemoteTerminalClient::dragLeaveEvent( QDragLeaveEvent* e )
{
	/*qDebug() << "2";
	QTextEdit::dragLeaveEvent( e );*/
}

void RemoteTerminalClient::dragMoveEvent( QDragMoveEvent* e )
{
	/*qDebug() << "3";
	QTextEdit::dragMoveEvent( e );*/
}

void RemoteTerminalClient::dropEvent( QDropEvent* e )
{
	/*qDebug() << "4";
	QTextEdit::dropEvent( e );*/
}

int RemoteTerminalClient::keyCode( Qt::Key key )
{
	switch( key )
	{
	case Qt::Key_Escape:
		return Control::KeyEscape;
	case Qt::Key_Tab:
		return Control::KeyTab;
	case Qt::Key_Backtab:
		return Control::KeyBacktab;
	case Qt::Key_Backspace:
		return Control::KeyBackspace;
	case Qt::Key_Return:
		return Control::KeyReturn;
	case Qt::Key_Enter:
		return Control::KeyEnter;
	case Qt::Key_Insert:
		return Control::KeyInsert;
	case Qt::Key_Delete:
		return Control::KeyDelete;
	case Qt::Key_Pause:
		return Control::KeyPause;
	case Qt::Key_Print:
		return Control::KeyPrint;
	case Qt::Key_SysReq:
		return Control::KeySysReq;
	case Qt::Key_Clear:
		return Control::KeyClear;
	case Qt::Key_Home:
		return Control::KeyHome;
	case Qt::Key_End:
		return Control::KeyEnd;
	case Qt::Key_Left:
		return Control::KeyLeft;
	case Qt::Key_Up:
		return Control::KeyUp;
	case Qt::Key_Right:
		return Control::KeyRight;
	case Qt::Key_Down:
		return Control::KeyDown;
	case Qt::Key_PageUp:
		return Control::KeyPageUp;
	case Qt::Key_PageDown:
		return Control::KeyPageDown;
	case Qt::Key_Shift:
		return Control::KeyShift;
	case Qt::Key_Control:
		return Control::KeyControl;
	case Qt::Key_Meta:
		return Control::KeyMeta;
	case Qt::Key_Alt:
		return Control::KeyAlt;
	case Qt::Key_CapsLock:
		return Control::KeyCapsLock;
	case Qt::Key_NumLock:
		return Control::KeyNumLock;
	case Qt::Key_ScrollLock:
		return Control::KeyScrollLock;
	case Qt::Key_F1:
		return Control::KeyF1;
	case Qt::Key_F2:
		return Control::KeyF2;
	case Qt::Key_F3:
		return Control::KeyF3;
	case Qt::Key_F4:
		return Control::KeyF4;
	case Qt::Key_F5:
		return Control::KeyF5;
	case Qt::Key_F6:
		return Control::KeyF6;
	case Qt::Key_F7:
		return Control::KeyF7;
	case Qt::Key_F8:
		return Control::KeyF8;
	case Qt::Key_F9:
		return Control::KeyF9;
	case Qt::Key_F10:
		return Control::KeyF10;
	case Qt::Key_F11:
		return Control::KeyF11;
	case Qt::Key_F12:
		return Control::KeyF12;
	default:
		break;
	}

	return -1;
}

void RemoteTerminalClient::moveText( QString& str )
{
	if( str.isEmpty() )
		return;
	if( replaceMode )
	{
		auto cur = textCursor();
		cur.movePosition( QTextCursor::MoveOperation::Right, QTextCursor::QTextCursor::KeepAnchor );
		cur.insertText( str, currentCharFormat() );
	}
	else
		textCursor().insertText( str, currentCharFormat() );
	str.clear();
}

void RemoteTerminalClient::processBytes()
{
	if( state == State::Disabled )
		return;

	while( device->bytesAvailable() )
	{
		switch( state )
		{
		case State::Sync:
		{
			if( device->bytesAvailable() < 2 )
				return;
			auto data = device->peek( 2 );
			if( ( uint8_t )data[0] != 255 || ( uint8_t )data[1] != Control::Sync )
				device->read( 1 );
			else
			{
				device->read( 2 );
				state = State::WaitPrefix;
				nextState = State::Input;
				break;
			}
			break;
		}
		case State::WaitPrefix:
		{
			do
			{
				char c = device->read( 1 )[0];
				if( c == '\n' )
				{
					if( nextState == State::Input )
					{
						setText( linePrefix );
						moveCursor( QTextCursor::MoveOperation::End );
						limitPosition = textCursor().position();
						setReadOnly( false );
					}
					state = nextState;
					break;
				}
				else
					linePrefix.append( c );
			} while( device->bytesAvailable() );
			break;
		}
		case State::Input:
		{
			device->readAll();
			break;
		}
		case State::WaitResponse:
		{
			if( device->bytesAvailable() < 2 )
				return;
			auto data = device->read( 2 );
			if( ( uint8_t )data[0] != 255 )
			{
				disable();
				device->readAll();
				return;
			}
			if( ( uint8_t )data[1] == Control::ProgramStarted )
			{
				append( "" );
				limitPosition = textCursor().position();
				setReadOnly( false );
				replaceMode = false;
				state = State::Exec;
			}
			else if( ( uint8_t )data[1] == Control::CommandNotFound )
			{
				errMessage.clear();
				state = State::WaitNotFoundCommandName;
			}
			else if( ( uint8_t )data[1] == Control::Ok )
			{
				append( linePrefix );
				moveCursor( QTextCursor::EndOfLine );
				limitPosition = textCursor().anchor();
				setReadOnly( false );
				state = State::Input;
			}
			else if( ( uint8_t )data[1] == Control::Error )
			{
				errMessage.clear();
				state = State::WaitErrorMessage;
			}
			else if( ( uint8_t )data[1] == Control::Prefix )
			{
				linePrefix.clear();
				state = State::WaitPrefix;
				nextState = State::WaitResponse;
			}
			else if( ( uint8_t )data[1] == Control::ConsoleClear )
			{
				clear();
				limitPosition = 0;
			}
			else
			{
				disable();
				device->readAll();
				return;
			}
			break;
		}
		case State::WaitErrorMessage:
		{
			do
			{
				char c = device->read( 1 )[0];
				if( c == '\n' )
				{
					append( errMessage );
					append( linePrefix );
					moveCursor( QTextCursor::EndOfLine );
					limitPosition = textCursor().anchor();
					setReadOnly( false );
					state = State::Input;
					break;
				}
				else
					errMessage.append( c );
			} while( device->bytesAvailable() );
			break;
		}
		case State::WaitNotFoundCommandName:
		{
			do
			{
				char c = device->read( 1 )[0];
				if( c == '\n' )
				{
					append( QString( "<Terminal>: command \'%1\' not found" ).arg( errMessage ) );
					append( linePrefix );
					moveCursor( QTextCursor::EndOfLine );
					limitPosition = textCursor().anchor();
					setReadOnly( false );
					state = State::Input;
					break;
				}
				else
					errMessage.append( c );
			} while( device->bytesAvailable() );
			break;
		}
		case State::Exec:
		case State::WaitTermination:
		{
			QString str;
			do
			{
				uint8_t c = ( uint8_t )device->peek( 1 )[0];
				if( c < 255 )
				{
					str.append( ( char )c );
					device->read( 1 );
				}
				else
				{
					if( device->bytesAvailable() < 2 )
					{
						moveText( str );
						return;
					}
					Control c = ( Control )( uint8_t )device->read( 2 )[1];
					if( c == RemoteTerminalClient::Char255 )
					{
						str.append( ( char )c );
						continue;
					}
					else
						moveText( str );
					switch( c )
					{
					case RemoteTerminalClient::ProgramFinished:
					{
						setCurrentCharFormat( QTextCharFormat() );
						append( linePrefix );
						moveCursor( QTextCursor::End );
						limitPosition = textCursor().anchor();
						setReadOnly( false );
						state = State::Input;

						uint8_t data[2] = { 255, Control::ProgramFinishedAck };
						device->write( ( const char* )data, 2 );
						goto Break;
					}
					case RemoteTerminalClient::CursorReplaceModeEnable:
					{
						replaceMode = true;
						goto Break;
					}
					case RemoteTerminalClient::CursorReplaceModeDisable:
					{
						replaceMode = false;
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveLeft:
					{
						if( textCursor().position() > limitPosition )
							moveCursor( QTextCursor::MoveOperation::Left );
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveRight:
					{
						moveCursor( QTextCursor::MoveOperation::Right );
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveN:
					{
						state = State::WaitMoveN;
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveToEnd:
					{
						moveCursor( QTextCursor::MoveOperation::End );
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveToBegin:
					{
						auto cur = textCursor();
						cur.setPosition( limitPosition );
						setTextCursor( cur );
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveToEndOfLine:
					{
						moveCursor( QTextCursor::MoveOperation::EndOfBlock );
						goto Break;
					}
					case RemoteTerminalClient::CursorMoveToBeginOfLine:
					{
						moveCursor( QTextCursor::MoveOperation::StartOfBlock );
						goto Break;
					}
					case RemoteTerminalClient::ConsoleDelete:
					{
						auto cur = textCursor();
						cur.movePosition( QTextCursor::MoveOperation::Right, QTextCursor::QTextCursor::KeepAnchor );
						cur.removeSelectedText();
						goto Break;
					}
					case RemoteTerminalClient::ConsoleBackspace:
					{
						if( textCursor().position() <= limitPosition )
							goto Break;
						auto cur = textCursor();
						cur.movePosition( QTextCursor::MoveOperation::Left, QTextCursor::QTextCursor::KeepAnchor );
						cur.removeSelectedText();
						goto Break;
					}
					case RemoteTerminalClient::ConsoleClear:
					{
						clear();
						limitPosition = 0;
						goto Break;
					}
					case RemoteTerminalClient::ConsoleSetTextColor:
					{
						state = State::WaitTextColor;
						goto Break;
					}
					case RemoteTerminalClient::ConsoleSetBackgroundColor:
					{
						state = State::WaitBackgroundColor;
						goto Break;
					}
					case RemoteTerminalClient::ConsoleRestoreColors:
					{
						setCurrentCharFormat( QTextCharFormat() );
						goto Break;
					}
					case RemoteTerminalClient::ConsoleCommit:
					{
						moveCursor( QTextCursor::End );
						limitPosition = textCursor().anchor();
						break;
					}
					default:
						disable();
						device->readAll();
						return;
					}
				}
			} while( device->bytesAvailable() );
			moveText( str );
		Break:
			break;
		}
		case State::WaitMoveN:
		{
			if( device->bytesAvailable() < 2 )
				return;
			int16_t n = ( int16_t )( ( ( uint16_t )device->read( 1 )[0] << 8 ) | ( ( uint16_t )device->read( 1 )[0] ) );
			QTextCursor cur = textCursor();
			if( n < 0 )
			{
				cur.movePosition( QTextCursor::MoveOperation::Left, QTextCursor::MoveMode::MoveAnchor, -n );
				if( cur.position() < limitPosition )
					cur.setPosition( limitPosition );
			}
			else
				cur.movePosition( QTextCursor::MoveOperation::Right, QTextCursor::MoveMode::MoveAnchor, n );
			setTextCursor( cur );
			state = State::Exec;
			break;
		}
		case State::WaitTextColor:
		case State::WaitBackgroundColor:
		{
			if( device->bytesAvailable() < 3 )
				return;
			uint8_t data[3];
			device->read( ( char* )data, 3 );
			if( state == State::WaitTextColor )
				setTextColor( QColor( data[0], data[1], data[2] ) );
			else
				setTextBackgroundColor( QColor( data[0], data[1], data[2] ) );
			state = State::Exec;
			break;
		}
		default:
			break;
		}
	}

	ensureCursorVisible();
}

void RemoteTerminalClient::aboutToClose()
{
	disable();
}
