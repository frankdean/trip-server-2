// -*- mode: js2; fill-column: 80; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// vim: set tw=80 ts=2 sts=0 sw=2 et ft=js norl:
/*
    This file is part of Trip Server 2, a program to support trip recording and
    itinerary planning.

    Copyright (C) 2022-2024 Frank Dean <frank.dean@fdsd.co.uk>

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

export { SelectionHandler };

class SelectionHandler {

  constructor(callback) {
    const self = this;
    this.callback = callback;
    this.update_page_nav_links();
    const list = document.getElementById('selection-list');
    self.cbxList = list.getElementsByTagName('input');
    for (let n = 0; n < self.cbxList.length; n++) {
      if (self.cbxList[n].getAttribute('type') === 'checkbox') {
        self.cbxList[n].addEventListener('change', (event) => {
          self.checkboxChangeEvent(event);
        }, false);
      }
    }
  }

  select_all() {
    const all = document.getElementsByTagName('input');
    for (let i = 0; i < all.length; i++) {
      if (all[i] !== this.selectAllCheckbox && all[i].type == 'checkbox') {
        all[i].checked = this.selectAllCheckbox.checked;
      }
    }
    this.update_page_nav_links();
    this.update_selection_nav_links();
  }

  update_page_nav_links() {
    const self = this;
    this.selectAllCheckbox = document.getElementById('input-select-all');
    this.selectAllCheckbox.addEventListener('click', () => {
      self.select_all();
    }, false);
    const selectAll = this.selectAllCheckbox.checked ? 'on' : '';
    const paging_div = document.getElementById('div-paging');
    if (paging_div) {
      const links = paging_div.getElementsByTagName('a');
      for (let n = 0; n < links.length; n++) {
        const href = links[n].getAttribute('href');
        const href2 = href.replace(/select-all=[^&]*/, 'select-all=' + selectAll);
        links[n].setAttribute('href', href2);
      }
    }
  }

  update_selection_nav_links() {
    const list = document.getElementById('selection-list');
    const links = list.getElementsByTagName('a');
    const selectAll = this.selectAllCheckbox.checked ? 'on' : '';
    for (let n = 0; n < links.length; n++) {
      const href = links[n].getAttribute('href');
      const href2 = href.replace(/select-all=[^&]*/, 'select-all=' + selectAll);
      links[n].setAttribute('href', href2);
    }
  }

  checkboxChangeEvent(event) {
    const element = event.srcElement;
    const name = element.getAttribute('name');
    if (name !== 'select-all') {
      this.checkboxValueChange(element);
    } else {
      for (let n = 0; n < this.cbxList.length; n++) {
        const cbx = this.cbxList[n];
        const boxName = cbx.getAttribute('name');
        if (boxName != 'select-all') {
          this.checkboxValueChange(cbx);
        }
      }
    }
  }

  checkboxValueChange(checkbox) {
    if (this.callback) {
      this.callback(checkbox.getAttribute('value'), checkbox.checked);
    }
  }

  isSelectAll() {
    return this.selectAllCheckbox.checked;
  }

} // SelectionHandler
