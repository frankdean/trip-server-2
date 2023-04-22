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
-- Data for Name: georef_format; Type: TABLE DATA; Schema: public; Owner: trip
--

COPY public.georef_format (key, value, ord) FROM stdin;
%d°%M′%S″%c	DMS+	1
%d°%M′%c	DM+	2
%d°%c	D+	3
%i%d	D	4
%p%d	±D	5
plus+code	OLC plus+code	6
osgb36	OS GB 1936 (BNG)	7
%dd%M'%S"%c	Proj4	8
%c%D° %M	QLandkarte GT	9
%c%d°%M′%S″	+DMS	10
%c%d°%M′\\	+DM	11
%c%d°	+D	12
%d° %M′ %S″ %c	D M S +	13
%d° %M′ %c	D M +	14
%d° %c	D +	15
%c %d° %M′ %S″	+ D M S	16
%c %d° %M′	+ D M	17
%c %d°	+ D	18
%d %m %s%c	Plain DMS+	18
%d %m%c	Plain DM+	19
%d%c	Plain D+	20
%c%d %m %s	Plain +DMS	21
%c%d %m	Plain +DM	22
%c%d	Plain +D	23
IrishGrid	Irish Grid	24
ITM	Irish Transverse Mercator	25
\.


--
-- PostgreSQL database dump complete
--

