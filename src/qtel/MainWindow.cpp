/**
@file	 MainWindow.cpp
@brief   Implementation class for the main window
@author  Tobias Blomberg
@date	 2003-03-09

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>

#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>
#include <QAction>
#include <QPixmap>
#include <QLabel>
#include <QToolTip>
#include <QPushButton>
#include <QInputDialog>
#include <QSplitter>
#include <QCloseEvent>
#undef emit


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <EchoLinkDirectory.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/QTEL.h"
#include "Settings.h"
#include "EchoLinkDispatcher.h"
#include "ComDialog.h"
#include "MainWindow.h"
#include "MsgHandler.h"
#include "EchoLinkDirectoryModel.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    MainWindow::MainWindow
 * Purpose:   Constructor
 * Input:     dir - An EchoLink directory server object
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-09
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
MainWindow::MainWindow(Directory &dir)
  : dir(dir), refresh_call_list_timer(0), is_busy(false), msg_handler(0),
    msg_audio_io(0), prev_status(StationData::STAT_UNKNOWN)
{
  setupUi(this);
  
  station_view->addAction(connectionConnectToSelectedAction);
  QAction *separator = new QAction(this);
  separator->setSeparator(true);
  station_view->addAction(separator);
  station_view->addAction(addNamedStationToBookmarksAction);
  station_view->addAction(addSelectedToBookmarksAction);
  station_view->addAction(removeSelectedFromBookmarksAction);

  connect(directoryRefreshAction, SIGNAL(activated()),
      	  this, SLOT(refreshCallList()));
  connect(connectionConnectToSelectedAction, SIGNAL(activated()),
      	  this, SLOT(connectionConnectToSelectedActionActivated()));
  connect(connectionConnectToIpAction, SIGNAL(activated()),
      	  this, SLOT(connectionConnectToIpActionActivated()));
  connect(addSelectedToBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(addSelectedToBookmarks()));
  connect(removeSelectedFromBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(removeSelectedFromBookmarks()));
  connect(addNamedStationToBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(addNamedStationToBookmarks()));
  connect(settingsConfigureAction, SIGNAL(activated()),
      	  this, SLOT(settings()));
  connect(helpAboutAction, SIGNAL(activated()),
      	  this, SLOT(helpAbout()));

  connect(station_view_selector, SIGNAL(currentItemChanged(QListWidgetItem*,
						           QListWidgetItem*)),
	  this, SLOT(stationViewSelectorCurrentItemChanged(QListWidgetItem*,
						           QListWidgetItem*)));
  connect(station_view, SIGNAL(activated(const QModelIndex&)),
	  this, SLOT(stationViewDoubleClicked(const QModelIndex&)));
  connect(incoming_con_view, SIGNAL(itemSelectionChanged()),
      	  this, SLOT(incomingSelectionChanged()));
  connect(incoming_con_view, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
      	  this, SLOT(acceptIncoming()));
  connect(incoming_clear_button, SIGNAL(clicked()),
      	  this, SLOT(clearIncomingList()));
  connect(incoming_accept_button, SIGNAL(clicked()),
      	  this, SLOT(acceptIncoming()));
  

  AudioIO::setChannels(1);
  
  dir.error.connect(slot(*this, &MainWindow::serverError));
  dir.statusChanged.connect(slot(*this, &MainWindow::statusChanged));
  dir.stationListUpdated.connect(
      slot(*this, &MainWindow::callsignListUpdated));
  
  EchoLink::Dispatcher *disp = EchoLink::Dispatcher::instance();
  disp->incomingConnection.connect(slot(*this, &MainWindow::incomingConnection));

  status_indicator = new QLabel(statusBar());
  status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
  statusBar()->addPermanentWidget(status_indicator);
    
  is_busy = Settings::instance()->startAsBusy();
  directoryBusyAction->setChecked(is_busy);
  updateRegistration();
  connect(directoryBusyAction, SIGNAL(toggled(bool)),
      	  this, SLOT(setBusy(bool)));
  
  //statusBar()->message(trUtf8("Getting calls from directory server..."));
  //dir.getCalls();
  refresh_call_list_timer = new QTimer(this);
  refresh_call_list_timer->start(
      1000 * 60 * Settings::instance()->listRefreshTime());
  connect(refresh_call_list_timer, SIGNAL(timeout()),
      this, SLOT(refreshCallList()));

  if (Settings::instance()->mainWindowSize().isValid())
  {
    resize(Settings::instance()->mainWindowSize());
    vsplitter->setSizes(Settings::instance()->vSplitterSizes());
    hsplitter->setSizes(Settings::instance()->hSplitterSizes());
  }
  
  Settings::instance()->configurationUpdated.connect(
      slot(*this, &MainWindow::configurationUpdated));
  
  bookmark_model = new EchoLinkDirectoryModel(this);
  conf_model = new EchoLinkDirectoryModel(this);
  link_model = new EchoLinkDirectoryModel(this);
  repeater_model = new EchoLinkDirectoryModel(this);
  station_model = new EchoLinkDirectoryModel(this);
  updateBookmarkModel();
  station_view_selector->setCurrentRow(0);
  
  initMsgAudioIo();
} /* MainWindow::MainWindow */


MainWindow::~MainWindow(void)
{
  delete msg_handler;
  msg_handler = 0;
  delete msg_audio_io;
  msg_audio_io = 0;
  Settings::instance()->setMainWindowSize(size());
  Settings::instance()->setHSplitterSizes(hsplitter->sizes());
  Settings::instance()->setVSplitterSizes(vsplitter->sizes());
  dir.makeOffline();
} /* MainWindow::~MainWindow */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void MainWindow::incomingConnection(const IpAddress& remote_ip,
    const string& remote_call, const string& remote_name,
    const string& remote_priv)
{
  time_t t = time(0);
  struct tm *tm = localtime(&t);
  char time_str[16];
  strftime(time_str, sizeof(time_str), "%H:%M", tm);
  
  QList<QTreeWidgetItem*> items = incoming_con_view->findItems(
		QString::fromStdString(remote_call),
		Qt::MatchExactly);
  QTreeWidgetItem *item = 0;
  if (items.size() == 0)
  {
    QStringList values;
    values << QString::fromStdString(remote_call)
           << QString::fromStdString(remote_name)
           << QString(time_str);
    item = new QTreeWidgetItem(values);
    msg_audio_io->open(AudioIO::MODE_WR);
    msg_handler->playFile(Settings::instance()->connectSound().toStdString());
  }
  else
  {
    Q_ASSERT(items.size() == 1);
    item = items.at(0);
    int item_index = incoming_con_view->indexOfTopLevelItem(item);
    Q_ASSERT(item_index >= 0);
    incoming_con_view->takeTopLevelItem(item_index);
    item->setText(2, time_str);
  }
  incoming_con_view->insertTopLevelItem(0, item);
  incoming_con_view->setCurrentItem(item);
  incoming_con_param[QString::fromStdString(remote_call)] =
      QString::fromStdString(remote_priv);
  
} /* MainWindow::incomingConnection */


void MainWindow::closeEvent(QCloseEvent *e)
{
  static int close_count = 0;
  
  if ((dir.status() != StationData::STAT_OFFLINE) && (close_count++ == 0))
  {
    statusBar()->showMessage(trUtf8("Logging off from directory server..."));
    dir.makeOffline();
    QTimer::singleShot(5000, this, SLOT(forceQuit()));
    e->ignore();
  }
  else
  {
    e->accept();
  }
} /* MainWindow::closeEvent */


void MainWindow::serverError(const string& msg)
{
  cout << msg << endl;
  statusBar()->showMessage(msg.c_str(), 5000);
  server_msg_view->append(msg.c_str());
} /* MainWindow::serverError */


void MainWindow::statusChanged(StationData::Status status)
{
  switch (status)
  {
    case StationData::STAT_ONLINE:
      status_indicator->setPixmap(
	  QPixmap(":/icons/images/online_icon.xpm"));
      if (prev_status != StationData::STAT_BUSY)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_BUSY:
      status_indicator->setPixmap(QPixmap(":/icons/images/busy_icon.xpm"));
      if (prev_status != StationData::STAT_ONLINE)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_OFFLINE:
      status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
      close();
      break;
      
    case StationData::STAT_UNKNOWN:
      status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
      break;
      
  }
  
  prev_status = status;
  
} /* MainWindow::statusChanged */


void MainWindow::allMsgsWritten(void)
{
  //cout << "MainWindow::allMsgsWritten\n";
  msg_audio_io->flushSamples();
} /* MainWindow::allMsgsWritten */


void MainWindow::allSamplesFlushed(void)
{
  //cout << "MainWindow::allSamplesFlushed\n";
  msg_audio_io->close();  
} /* MainWindow::allSamplesFlushed */


void MainWindow::initMsgAudioIo(void)
{
  delete msg_audio_io;
  delete msg_handler;
  
  msg_audio_io =
      new AudioIO(Settings::instance()->spkrAudioDevice().toStdString(), 0);
  msg_handler = new MsgHandler(msg_audio_io->sampleRate());
  msg_handler->allMsgsWritten.connect(slot(*this, &MainWindow::allMsgsWritten));
  msg_audio_io->registerSource(msg_handler);
  
} /* MainWindow::initMsgAudioIo */


void MainWindow::updateBookmarkModel(void)
{
  list<StationData> bookmarks;
  QStringList callsigns = Settings::instance()->bookmarks();
  QStringList::iterator it;
  foreach (QString callsign, callsigns)
  {
    const StationData *station = dir.findCall(callsign.toStdString());
    if (station != 0)
    {
      bookmarks.push_back(*station);
    }
    else
    {
      StationData stn;
      stn.setCallsign(callsign.toStdString());
      stn.setStatus(StationData::STAT_OFFLINE);
      stn.setIp(Async::IpAddress("0.0.0.0"));
      bookmarks.push_back(stn);
    }
  }
  bookmark_model->updateStationList(bookmarks);
  
} /* MainWindow::updateBookmarkModel */



void MainWindow::stationViewSelectorCurrentItemChanged(QListWidgetItem *current,
						       QListWidgetItem *previous
						      )
{
  Q_UNUSED(previous);
  
  if (current == 0)
  {
    return;
  }
  
  QAbstractItemModel *model = 0;
  QItemSelectionModel *m = station_view->selectionModel();
  if (current->text() == trUtf8("Bookmarks"))
  {
    model = bookmark_model;
  }
  else if (current->text() == trUtf8("Conferences"))
  {
    model = conf_model;
  }
  else if (current->text() == trUtf8("Links"))
  {
    model = link_model;
  }
  else if (current->text() == trUtf8("Repeaters"))
  {
    model = repeater_model;
  }
  else if (current->text() == trUtf8("Stations"))
  {
    model = station_model;
  }
  
  station_view->setModel(model);
  if (m != 0)
  {
    m->clear();
    delete m;
  }
  
  QItemSelectionModel *sm = station_view->selectionModel();
  if (sm != 0)
  {
    connect(sm, SIGNAL(selectionChanged(const QItemSelection&,
					const QItemSelection&)),
	    this, SLOT(stationViewSelectionChanged(const QItemSelection&,
						   const QItemSelection&)));
  }

} /* MainWindow::stationViewSelectorCurrentItemChanged */


void MainWindow::stationViewDoubleClicked(const QModelIndex &index)
{
  ComDialog *com_dialog = new ComDialog(dir, index.data(0).toString(), "?");
  com_dialog->show();
} /* MainWindow::stationViewDoubleClicked */


void MainWindow::stationViewSelectionChanged(const QItemSelection &current,
					     const QItemSelection &previous)
{
  Q_UNUSED(previous);
  
  QModelIndexList indexes = current.indexes();
  bool item_selected = ((indexes.count() > 0) && indexes.at(0).isValid() &&
                        (indexes.at(0).column() == 0));
  
  connectionConnectToSelectedAction->setEnabled(item_selected);
  addSelectedToBookmarksAction->setEnabled(item_selected);
  removeSelectedFromBookmarksAction->setEnabled(item_selected);
  
} /* MainWindow::stationViewSelectionChanged */


void MainWindow::callsignListUpdated(void)
{
  updateBookmarkModel();
  
  conf_model->updateStationList(dir.conferences());
  link_model->updateStationList(dir.links());
  repeater_model->updateStationList(dir.repeaters());
  station_model->updateStationList(dir.stations());
  
  statusBar()->showMessage(trUtf8("Station list has been refreshed"), 5000);
  
  const string &msg = dir.message();
  if (msg != old_server_msg)
  {
    server_msg_view->append(msg.c_str());
    old_server_msg = msg;
  }
    
} /* MainWindow::callsignListUpdated */


void MainWindow::refreshCallList(void)
{
  statusBar()->showMessage(trUtf8("Refreshing station list..."));
  dir.getCalls();
} /* MainWindow::refreshCallList */


void MainWindow::updateRegistration(void)
{
  if (is_busy)
  {
    dir.makeBusy();
  }
  else
  {
    dir.makeOnline();
  }
} /* MainWindow::updateRegistration */


void MainWindow::setBusy(bool busy)
{
  is_busy = busy;
  updateRegistration();
} /* MainWindow::setBusy */


void MainWindow::forceQuit(void)
{
  close();
} /* MainWindow::forceQuit */


void MainWindow::incomingSelectionChanged(void)
{
  incoming_accept_button->setEnabled(incoming_con_view->selectedItems().size() > 0);
} /* MainWindow::incomingSelectionChanged */


void MainWindow::clearIncomingList(void)
{
  incoming_con_view->clear();
  incoming_accept_button->setEnabled(FALSE);
} /* MainWindow::clearIncomingList */


void MainWindow::acceptIncoming(void)
{
  QList<QTreeWidgetItem*> items = incoming_con_view->selectedItems();
  Q_ASSERT(items.size() == 1);
  QTreeWidgetItem *item = items.at(0);
  incoming_accept_button->setEnabled(FALSE);
  ComDialog *com_dialog = new ComDialog(dir, item->text(0), item->text(1));
  com_dialog->show();
  com_dialog->acceptConnection();
  com_dialog->setRemoteParams(incoming_con_param[item->text(0)]);
  int item_index = incoming_con_view->indexOfTopLevelItem(item);
  Q_ASSERT(item_index >= 0);
  incoming_con_view->takeTopLevelItem(item_index);
  delete item;
} /* MainWindow::acceptIncoming */


void MainWindow::addSelectedToBookmarks(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  QStringList bookmarks = Settings::instance()->bookmarks();
  if (!callsign.isEmpty() && !bookmarks.contains(callsign))
  {
    bookmarks.append(callsign);
    Settings::instance()->setBookmarks(bookmarks);
  }
  updateBookmarkModel();

} /* MainWindow::addSelectedToBookmarks */


void MainWindow::removeSelectedFromBookmarks(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || !indexes.at(0).isValid() ||
      (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  QStringList bookmarks = Settings::instance()->bookmarks();
  if (!callsign.isEmpty() && bookmarks.contains(callsign))
  {
    bookmarks.removeAll(callsign);
    Settings::instance()->setBookmarks(bookmarks);
    updateBookmarkModel();
  }
} /* MainWindow::removeSelectedFromBookmarks */


void MainWindow::addNamedStationToBookmarks(void)
{
  QString call = QInputDialog::getText(this, trUtf8("Qtel - Add station..."),
      trUtf8("Enter callsign of the station to add"));
  
  if (!call.isEmpty())
  {
    call = call.toUpper();
    QStringList bookmarks = Settings::instance()->bookmarks();
    if (!bookmarks.contains(call))
    {
      bookmarks.append(call);
      Settings::instance()->setBookmarks(bookmarks);
      updateBookmarkModel();
    }
  }
  
} /* MainWindow::addNamedStationToBookmarks */


void MainWindow::configurationUpdated(void)
{
  dir.setServer(Settings::instance()->directoryServer().toStdString());
  dir.setCallsign(Settings::instance()->callsign().toStdString());
  dir.setPassword(Settings::instance()->password().toStdString());
  dir.setDescription(Settings::instance()->location().toStdString());
  updateRegistration();
  
  refresh_call_list_timer->setInterval(
      1000 * 60 * Settings::instance()->listRefreshTime());
  
  initMsgAudioIo();
} /* MainWindow::configurationChanged */


void MainWindow::connectionConnectToIpActionActivated(void)
{
  bool ok;
  QString remote_host = QInputDialog::getText(
	    this,
	    trUtf8("Qtel: Connect to IP"),
	    trUtf8("Enter an IP address or hostname:"),
	    QLineEdit::Normal, Settings::instance()->connectToIp(), &ok);
  if (ok)
  {
    Settings::instance()->setConnectToIp(remote_host);
    if (!remote_host.isEmpty())
    {
      ComDialog *com_dialog = new ComDialog(dir, remote_host);
      com_dialog->show();
    }
  }
} /* MainWindow::connectionConnectToIpActionActivated */


void MainWindow::connectionConnectToSelectedActionActivated(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  if (!callsign.isEmpty())
  {
    ComDialog *com_dialog = new ComDialog(dir, callsign, "?");
    com_dialog->show();
  }
} /* MainWindow::connectionConnectToSelectedActionActivated */


void MainWindow::settings(void)
{
  Settings::instance()->showDialog();
} /* MainWindow::settings */


void MainWindow::helpAbout(void)
{
    QMessageBox::about(this, trUtf8("About Qtel"),
        trUtf8("Qtel v") + QTEL_VERSION +
        trUtf8(" - Qt EchoLink client.\n") +
        "\n" +
        trUtf8("Copyright (C) 2011 Tobias Blomberg / SM0SVX\n\n"
               "Qtel comes with ABSOLUTELY NO WARRANTY. "
               "This is free software, and you "
               "are welcome to redistribute it in accordance with the "
               "terms and conditions in "
               "the GNU GPL (General Public License) version 2 or later."));
} /* MainWindow::helpAbout */



/*
 * This file has not been truncated
 */

