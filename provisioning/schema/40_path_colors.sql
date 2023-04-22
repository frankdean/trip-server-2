--
-- PostgreSQL database dump
--

-- Dumped from database version 11.7 (Debian 11.7-2.pgdg90+1)
-- Dumped by pg_dump version 11.7 (Debian 11.7-2.pgdg90+1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Data for Name: path_color; Type: TABLE DATA; Schema: public; Owner: trip
--

COPY public.path_color (key, value, html_code) FROM stdin;
Black	Black	black
White	White	white
Red	Red	red
Yellow	Yellow	yellow
Blue	Blue	blue
Magenta	Magenta	fuchsia
Cyan	Cyan	aqua
DarkRed	Dark Red	maroon
Green	Green	lime
DarkGreen	Dark Green	green
LightGray	Light Gray	silver
DarkGray	Dark Gray	gray
DarkBlue	Dark Blue	navy
DarkMagenta	Dark Magenta	purple
DarkYellow	Dark Yellow	olive
DarkCyan	Dark Cyan	teal
\.


--
-- PostgreSQL database dump complete
--

