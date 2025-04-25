// -*- mode: c++; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=cpp norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2025 Frank Dean <frank.dean@fdsd.co.uk>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Affero General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
    for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef TEXT_USER_INTERFACE_HPP
#define TEXT_USER_INTERFACE_HPP

#include <memory>
#include <final/final.h>

namespace fdsd
{
namespace trip
{

class UserEditDialog : public finalcut::FDialog {
  finalcut::FLineEdit input_email{this};
  finalcut::FLineEdit input_firstname{this};
  finalcut::FLineEdit input_lastname{this};
  finalcut::FLineEdit input_nickname{this};
  finalcut::FLineEdit input_password{this};
  finalcut::FCheckBox input_admin{this};
  finalcut::FButton btn_ok{this};
  finalcut::FButton btn_cancel{this};
  void save();
  void cancel();
  std::size_t column1_width;
  bool invalid;
  bool cancelled;
  static int count;
public:
  explicit UserEditDialog(finalcut::FWidget* = nullptr);
  std::string get_email() {
    return input_email.getText().toString();
  }
  std::string get_firstname() {
    return input_firstname.getText().toString();
  }
  std::string get_lastname() {
    return input_lastname.getText().toString();
  }
  std::string get_nickname() {
    return input_nickname.getText().toString();
  }
  std::string get_password() {
    return input_password.getText().toString();
  }
  bool is_admin() {
    return input_admin.isChecked();
  }
  bool is_invalid() {
    return invalid;
  }
  bool is_cancelled() {
    return cancelled;
  }
};

class TripMenu final : public finalcut::FDialog {
public:
  explicit TripMenu(finalcut::FWidget* = nullptr);
protected:
  void add_user_option();
private:
  struct UserMenu {
    explicit UserMenu(finalcut::FMenuBar& menubar);
    finalcut::FMenu user;
    finalcut::FMenuItem add_user;
    finalcut::FMenuItem separator1;
    finalcut::FMenuItem quit;
  };
  void configureUserMenu();
  void initLayout() override;
  void adjustSize() override;

  finalcut::FMenuBar menubar{this};
  UserMenu user_menu{menubar};
  finalcut::FStatusBar status_bar{this};
  finalcut::FLabel column1{this};
  finalcut::FLabel column2{this};
  finalcut::FLabel instructions{this};

  int column1_title_width;
  int column2_title_width;
  int column2_width;

};

class TextUserInterface {
public:
  TextUserInterface() {};
  int run(int argc, char* argv[]);
};

}// namespace trip
}// namespace fdsd

#endif // TEXT_USER_INTERFACE_HPP
