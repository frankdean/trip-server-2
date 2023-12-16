// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2023 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "../config.h"
#include "session_pg_dao.hpp"
#include "text-based_user_interface.hpp"
#include "../trip-server-common/src/uuid.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#include <utility>
#include <boost/locale.hpp>

using namespace fdsd::trip;
using namespace boost::locale;
using namespace finalcut;

TripMenu::TripMenu (finalcut::FWidget* parent)
  : finalcut::FDialog{parent},
    column1_title_width(0),
    column2_title_width(0),
    column2_width(0)
{
  // Message shown on status bar when the 'User' menu option is active
  user_menu.user.setStatusbarMessage(translate("User commands").str());

  configureUserMenu();

  // The 'Key' column heading for instructions to the user on which keys perform
  // which function
  FString column1_heading(translate("Key").str());
  column1 << ' ' << column1_heading;
  column1.ignorePadding();
  column1.setEmphasis();

  // The 'Function' column heading for instructions to the user on which keys
  // perform which function
  FString column2_heading(translate("Function").str());
  column2 << ' ' << column2_heading;
  column2.ignorePadding();
  column2.setEmphasis();

  finalcut::FString instruction_lines[3];
  // Describes the behaviour of the <F10> function key
  // Describes the behaviour of the <Ctrl>+<Space> key combination
  // Describes the behaviour of the <Menu> function
  instruction_lines[0] = translate("Activate menu bar").str();
  // Describes the behaviour of the <Shift>+<Menu> key combination
  instruction_lines[1] = translate("Open dialog menu").str();
  // Describes the behaviour of the <Ctrl>+<q> key combination
  instruction_lines[2] = translate("Quit").str();

  instructions << "<F10>            "
               << instruction_lines[0] << '\n'
               << "<Ctrl>+<Space>   "
               << instruction_lines[0] << '\n'
               << "<Menu>           "
               << instruction_lines[0] << '\n'
               << "<Shift>+<Menu>   "
               << instruction_lines[1] << '\n'
               << "<Ctrl>+<q>       "
               << instruction_lines[2] << '\n';

  column1_title_width = column1_heading.getLength();
  column2_title_width = column2_heading.getLength();
  // Initilise to the width of the first column
  column2_width = column2_title_width;
  for (auto line : instruction_lines)
    column2_width = std::max(static_cast<int>(line.getLength()), column2_width);

  const int column1_width = 17;
  column2_width += column1_width + 2;
  
  // Title of application shown on interactive window
  setText("Trip");
  setSize({static_cast<std::size_t>(column2_width + 4), 8});
}

void TripMenu::configureUserMenu()
{
  user_menu.add_user.addAccelerator(FKey::Ctrl_n);
  // Message shown on status bar when the 'Add User' menu option is active
  user_menu.add_user.setStatusbarMessage(translate("Create a new user").str());
  user_menu.separator1.setSeparator();
  user_menu.quit.addAccelerator(FKey::Ctrl_q);
  // Message shown on status bar when the 'Quit' menu option is active
  user_menu.quit.setStatusbarMessage(translate("Quit the program").str());

  user_menu.add_user.addCallback(
      "clicked",
      this,
      &TripMenu::add_user_option
    );

  user_menu.quit.addCallback(
      "clicked",
      finalcut::getFApplication(),
      &finalcut::FApplication::cb_exitApp,
      this);
}

void TripMenu::initLayout()
{
  column1.setGeometry(FPoint{3, 2},
                      FSize{static_cast<std::size_t>(column1_title_width + 2), 1});
  column2.setGeometry(FPoint{19, 2},
                      FSize{static_cast<std::size_t>(column2_title_width + 2), 1});
  instructions.setGeometry(
      FPoint{2, 1},
      FSize{static_cast<std::size_t>(column2_width), 5});

  FDialog::initLayout();
}

void TripMenu::adjustSize()
{
  const int pw = getDesktopWidth();
  const int ph = getDesktopHeight();
  setX(1 + (pw - int(getWidth())) / 2, false);
  setY(1 + (ph - int(getHeight())) / 4, false);
  FDialog::adjustSize();
}

void TripMenu::add_user_option()
{
  UserEditDialog user_edit_dialog(this);
  user_edit_dialog.setTransparentShadow();
  user_edit_dialog.setModal();
  user_edit_dialog.show();
  if (!user_edit_dialog.is_cancelled()) {
    SessionPgDao::user user;
    user.uuid = std::make_pair(true, utils::UUID::generate_uuid());
    user.is_admin = true;
    user.email = user_edit_dialog.get_email();
    user.firstname = user_edit_dialog.get_firstname();
    user.lastname = user_edit_dialog.get_lastname();
    user.nickname = user_edit_dialog.get_nickname();
    user.password.first = true;
    user.password.second = user_edit_dialog.get_password();
    try  {
      SessionPgDao dao;
      dao.save(user);
      // FMessageBox::info(this,
      //                   translate("Save").str(),
      //                   translate("Saved!").str());
      status_bar.setStatusbarMessage(translate("User saved!").str());
    } catch (const pqxx::failure& e) {
      std::ostringstream os;
      os << typeid(e).name() << " exception: " << e.what();
      FMessageBox::error(this,
                         e.what());
    }
  }
}

TripMenu::UserMenu::UserMenu(FMenuBar& menubar)
  : user(boost::locale::translate("&User").str(), &menubar),
    add_user(translate("&Add User").str(), &user),
    separator1(&user),
    quit(translate("&Quit").str(), &user)
{ }

int UserEditDialog::count = 0;

UserEditDialog::UserEditDialog (finalcut::FWidget* parent)
  : finalcut::FDialog(parent),
    column1_width(0),
    invalid(true),
    cancelled(false)
{
  // Title of the OK button on the text-based UI
  const std::string ok_label = translate("&OK").str();
  // Title of the Cancel button on the text-based UI
  const std::string cancel_label = translate("&Cancel").str();
  // Prompt for email input on text-based UI
  const std::string email_label = translate("&Email");
  // Prompt for user's first name on text-based UI
  const std::string firstname_label = translate("&First name");
  // Prompt for user's last name on text-based UI
  const std::string lastname_label = translate("&Last name");
  // Prompt for nickname input on text-based UI
  const std::string nickname_label = translate("&Nickname");
  const std::string password_label = translate("&Password").str();
  const bool shadow_inputs = false;

  column1_width = std::max({
      nickname_label.length(),
      email_label.length(),
      firstname_label.length(),
      lastname_label.length(),
      password_label.length()
    });
  auto dialog_width = column1_width + 50;

  setText(translate("Create user").str());
  setGeometry(FPoint(4, 2), FSize(dialog_width, 17));
  input_email.setLabelText(email_label);
  input_email.setGeometry(
      FPoint(column1_width + 3, 2),
      FSize(column1_width + 32, 1));
  input_email.setShadow(shadow_inputs);
  input_firstname.setLabelText(firstname_label);
  input_firstname.setGeometry(
      FPoint(column1_width + 3, 4),
      FSize(column1_width + 32, 1));
  input_firstname.setShadow(shadow_inputs);
  input_lastname.setLabelText(lastname_label);
  input_lastname.setGeometry(
      FPoint(column1_width + 3, 6),
      FSize(column1_width + 32, 1));
  input_lastname.setShadow(shadow_inputs);
  input_nickname.setLabelText(nickname_label);
  input_nickname.setGeometry(FPoint(column1_width + 3, 8), FSize(20, 1));
  input_nickname.setShadow(shadow_inputs);
  input_password.setLabelText(password_label);
  input_password.setInputType(FLineEdit::InputType::Password);
  input_password.setGeometry(FPoint(column1_width + 3, 10), FSize(20, 1));
  input_password.setShadow(shadow_inputs);

  btn_cancel.setText(cancel_label);
  btn_cancel.setGeometry(
      FPoint(dialog_width / 2 - cancel_label.length() - 6, 13),
      FSize(cancel_label.length() + 4, 1));

  btn_ok.setText(ok_label);
  btn_ok.setGeometry(
      FPoint(dialog_width / 2 + 6, 13),
      FSize(ok_label.length() + 4, 1));
  btn_ok.addCallback(
      "clicked",
      this,
      &UserEditDialog::save
    );
  btn_cancel.addCallback(
      "clicked",
      this,
      &UserEditDialog::cancel
    );
  btn_ok.setShadow(true);
}

void UserEditDialog::save()
{
  invalid = get_email().empty() || get_firstname().empty() ||
    get_lastname().empty() || get_nickname().empty() ||
    get_password().empty();

  if (invalid) {
    auto buttonType = FMessageBox::error(
        this,
        translate("You must enter a value for each field").str(),
        FMessageBox::ButtonType::Ok,
        FMessageBox::ButtonType::Reject);
    if (buttonType == FMessageBox::ButtonType::Reject)
      cancelled = true;
  } else {
    hide();
  }
}

void UserEditDialog::cancel()
{
  cancelled = true;
  hide();
}

int TextUserInterface::run(int argc, char* argv[])
{
  FApplication app{argc, argv};
  TripMenu main_dialog(&app);

  FWidget::setMainWidget(&main_dialog);

  main_dialog.show();
  return app.exec();
}
