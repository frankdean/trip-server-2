--
-- PostgreSQL database dump
--

-- Dumped from database version 13.5
-- Dumped by pg_dump version 13.5

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
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- Name: pgcrypto; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pgcrypto WITH SCHEMA public;


--
-- Name: EXTENSION pgcrypto; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';


--
-- Name: postgis; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS postgis WITH SCHEMA public;


--
-- Name: EXTENSION postgis; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION postgis IS 'PostGIS geometry, geography, and raster spatial types and functions';


SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: georef_format; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.georef_format (
    key text NOT NULL,
    value text NOT NULL,
    ord integer NOT NULL
);


ALTER TABLE public.georef_format OWNER TO trip;

--
-- Name: itinerary; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary (
    id integer NOT NULL,
    title text NOT NULL,
    description text,
    user_id integer NOT NULL,
    archived boolean DEFAULT false,
    start timestamp with time zone,
    finish timestamp with time zone
);


ALTER TABLE public.itinerary OWNER TO trip;

--
-- Name: itinerary_route; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_route (
    id integer NOT NULL,
    itinerary_id integer NOT NULL,
    name text,
    distance numeric(12,2),
    ascent numeric(9,1),
    descent numeric(9,1),
    lowest numeric(8,1),
    highest numeric(8,1),
    color text
);


ALTER TABLE public.itinerary_route OWNER TO trip;

--
-- Name: itinerary_route_point; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_route_point (
    id integer NOT NULL,
    itinerary_route_id integer NOT NULL,
    name text,
    comment text,
    description text,
    symbol text,
    altitude numeric(11,5),
    geog public.geography(Point,4326) NOT NULL
);


ALTER TABLE public.itinerary_route_point OWNER TO trip;

--
-- Name: itinerary_route_point_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_route_point_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_route_point_seq OWNER TO trip;

--
-- Name: itinerary_route_point_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_route_point_seq OWNED BY public.itinerary_route_point.id;


--
-- Name: itinerary_route_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_route_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_route_seq OWNER TO trip;

--
-- Name: itinerary_route_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_route_seq OWNED BY public.itinerary_route.id;


--
-- Name: itinerary_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_seq OWNER TO trip;

--
-- Name: itinerary_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_seq OWNED BY public.itinerary.id;


--
-- Name: itinerary_sharing; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_sharing (
    itinerary_id integer NOT NULL,
    shared_to_id integer NOT NULL,
    active boolean
);


ALTER TABLE public.itinerary_sharing OWNER TO trip;

--
-- Name: itinerary_track; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_track (
    id integer NOT NULL,
    itinerary_id integer NOT NULL,
    name text,
    color text,
    distance numeric(12,2),
    ascent numeric(9,1),
    descent numeric(9,1),
    lowest numeric(8,1),
    highest numeric(8,1)
);


ALTER TABLE public.itinerary_track OWNER TO trip;

--
-- Name: itinerary_track_point; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_track_point (
    id integer NOT NULL,
    itinerary_track_segment_id integer NOT NULL,
    "time" timestamp with time zone,
    hdop numeric(6,1),
    altitude numeric(11,5),
    geog public.geography(Point,4326) NOT NULL
);


ALTER TABLE public.itinerary_track_point OWNER TO trip;

--
-- Name: itinerary_track_point_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_track_point_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_track_point_seq OWNER TO trip;

--
-- Name: itinerary_track_point_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_track_point_seq OWNED BY public.itinerary_track_point.id;


--
-- Name: itinerary_track_segment; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_track_segment (
    id integer NOT NULL,
    itinerary_track_id integer NOT NULL,
    distance numeric(12,2),
    ascent numeric(9,1),
    descent numeric(9,1),
    lowest numeric(8,1),
    highest numeric(8,1)
);


ALTER TABLE public.itinerary_track_segment OWNER TO trip;

--
-- Name: itinerary_track_segment_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_track_segment_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_track_segment_seq OWNER TO trip;

--
-- Name: itinerary_track_segment_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_track_segment_seq OWNED BY public.itinerary_track_segment.id;


--
-- Name: itinerary_track_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_track_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_track_seq OWNER TO trip;

--
-- Name: itinerary_track_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_track_seq OWNED BY public.itinerary_track.id;


--
-- Name: itinerary_waypoint; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.itinerary_waypoint (
    id integer NOT NULL,
    itinerary_id integer NOT NULL,
    name text,
    "time" timestamp with time zone,
    comment text,
    symbol text,
    altitude numeric(11,5),
    description text,
    color text,
    type text,
    avg_samples integer,
    geog public.geography(Point,4326) NOT NULL
);


ALTER TABLE public.itinerary_waypoint OWNER TO trip;

--
-- Name: itinerary_waypoint_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.itinerary_waypoint_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.itinerary_waypoint_seq OWNER TO trip;

--
-- Name: itinerary_waypoint_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.itinerary_waypoint_seq OWNED BY public.itinerary_waypoint.id;


--
-- Name: location_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.location_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.location_seq OWNER TO trip;

--
-- Name: location; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.location (
    id integer DEFAULT nextval('public.location_seq'::regclass) NOT NULL,
    user_id integer NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    hdop numeric(6,1),
    altitude numeric(11,5),
    speed numeric(6,1),
    bearing numeric(11,5),
    sat smallint,
    provider text,
    battery numeric(4,1),
    note text,
    geog public.geography(Point,4326) NOT NULL
);


ALTER TABLE public.location OWNER TO trip;

--
-- Name: location_sharing; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.location_sharing (
    shared_by_id integer NOT NULL,
    shared_to_id integer NOT NULL,
    recent_minutes integer,
    max_minutes integer,
    active boolean
);


ALTER TABLE public.location_sharing OWNER TO trip;

--
-- Name: path_color; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.path_color (
    key text NOT NULL,
    value text NOT NULL,
    html_code text
);


ALTER TABLE public.path_color OWNER TO trip;

--
-- Name: role; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.role (
    id integer NOT NULL,
    name text NOT NULL
);


ALTER TABLE public.role OWNER TO trip;

--
-- Name: role_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.role_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.role_seq OWNER TO trip;

--
-- Name: role_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: trip
--

ALTER SEQUENCE public.role_seq OWNED BY public.role.id;


--
-- Name: session; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.session (
    id uuid NOT NULL,
    user_id integer NOT NULL,
    updated timestamp without time zone NOT NULL
);


ALTER TABLE public.session OWNER TO trip;

--
-- Name: session_data; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.session_data (
    session_id uuid NOT NULL,
    key text NOT NULL,
    value text
);


ALTER TABLE public.session_data OWNER TO trip;

--
-- Name: tile; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.tile (
    server_id integer DEFAULT 0 NOT NULL,
    x integer NOT NULL,
    y integer NOT NULL,
    z smallint NOT NULL,
    image bytea,
    updated timestamp without time zone DEFAULT now() NOT NULL,
    expires timestamp without time zone NOT NULL
);


ALTER TABLE public.tile OWNER TO trip;

--
-- Name: tile_download_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.tile_download_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.tile_download_seq OWNER TO trip;

--
-- Name: tile_metric; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.tile_metric (
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    count integer NOT NULL
);


ALTER TABLE public.tile_metric OWNER TO trip;

--
-- Name: user_role; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.user_role (
    user_id integer NOT NULL,
    role_id integer NOT NULL
);


ALTER TABLE public.user_role OWNER TO trip;

--
-- Name: usertable_seq; Type: SEQUENCE; Schema: public; Owner: trip
--

CREATE SEQUENCE public.usertable_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.usertable_seq OWNER TO trip;

--
-- Name: usertable; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.usertable (
    id integer DEFAULT nextval('public.usertable_seq'::regclass) NOT NULL,
    firstname text NOT NULL,
    lastname text NOT NULL,
    email text NOT NULL,
    uuid uuid NOT NULL,
    password text NOT NULL,
    nickname text NOT NULL,
    tl_settings text
);


ALTER TABLE public.usertable OWNER TO trip;

--
-- Name: waypoint_symbol; Type: TABLE; Schema: public; Owner: trip
--

CREATE TABLE public.waypoint_symbol (
    key text NOT NULL,
    value text NOT NULL
);


ALTER TABLE public.waypoint_symbol OWNER TO trip;

--
-- Name: itinerary id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary ALTER COLUMN id SET DEFAULT nextval('public.itinerary_seq'::regclass);


--
-- Name: itinerary_route id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route ALTER COLUMN id SET DEFAULT nextval('public.itinerary_route_seq'::regclass);


--
-- Name: itinerary_route_point id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route_point ALTER COLUMN id SET DEFAULT nextval('public.itinerary_route_point_seq'::regclass);


--
-- Name: itinerary_track id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track ALTER COLUMN id SET DEFAULT nextval('public.itinerary_track_seq'::regclass);


--
-- Name: itinerary_track_point id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_point ALTER COLUMN id SET DEFAULT nextval('public.itinerary_track_point_seq'::regclass);


--
-- Name: itinerary_track_segment id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_segment ALTER COLUMN id SET DEFAULT nextval('public.itinerary_track_segment_seq'::regclass);


--
-- Name: itinerary_waypoint id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_waypoint ALTER COLUMN id SET DEFAULT nextval('public.itinerary_waypoint_seq'::regclass);


--
-- Name: role id; Type: DEFAULT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.role ALTER COLUMN id SET DEFAULT nextval('public.role_seq'::regclass);


--
-- Name: georef_format georef_format_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.georef_format
    ADD CONSTRAINT georef_format_pkey PRIMARY KEY (key);


--
-- Name: georef_format georef_format_value_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.georef_format
    ADD CONSTRAINT georef_format_value_key UNIQUE (value);


--
-- Name: itinerary itinerary_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary
    ADD CONSTRAINT itinerary_pkey PRIMARY KEY (id);


--
-- Name: itinerary_route itinerary_route_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route
    ADD CONSTRAINT itinerary_route_pkey PRIMARY KEY (id);


--
-- Name: itinerary_route_point itinerary_route_point_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route_point
    ADD CONSTRAINT itinerary_route_point_pkey PRIMARY KEY (id);


--
-- Name: itinerary_sharing itinerary_sharing_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_sharing
    ADD CONSTRAINT itinerary_sharing_pkey PRIMARY KEY (itinerary_id, shared_to_id);


--
-- Name: itinerary_track itinerary_track_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track
    ADD CONSTRAINT itinerary_track_pkey PRIMARY KEY (id);


--
-- Name: itinerary_track_point itinerary_track_point_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_point
    ADD CONSTRAINT itinerary_track_point_pkey PRIMARY KEY (id);


--
-- Name: itinerary_track_segment itinerary_track_segment_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_segment
    ADD CONSTRAINT itinerary_track_segment_pkey PRIMARY KEY (id);


--
-- Name: itinerary_waypoint itinerary_waypoint_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_waypoint
    ADD CONSTRAINT itinerary_waypoint_pkey PRIMARY KEY (id);


--
-- Name: location location_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.location
    ADD CONSTRAINT location_pkey PRIMARY KEY (id);


--
-- Name: location_sharing location_sharing_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.location_sharing
    ADD CONSTRAINT location_sharing_pkey PRIMARY KEY (shared_by_id, shared_to_id);


--
-- Name: role role_name_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.role
    ADD CONSTRAINT role_name_key UNIQUE (name);


--
-- Name: role role_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.role
    ADD CONSTRAINT role_pkey PRIMARY KEY (id);


--
-- Name: session_data session_data_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.session_data
    ADD CONSTRAINT session_data_pkey PRIMARY KEY (session_id, key);


--
-- Name: session session_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.session
    ADD CONSTRAINT session_pkey PRIMARY KEY (id);


--
-- Name: tile tile_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.tile
    ADD CONSTRAINT tile_pkey PRIMARY KEY (server_id, x, y, z);


--
-- Name: path_color track_color_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.path_color
    ADD CONSTRAINT track_color_pkey PRIMARY KEY (key);


--
-- Name: path_color track_color_value_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.path_color
    ADD CONSTRAINT track_color_value_key UNIQUE (value);


--
-- Name: user_role user_role_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.user_role
    ADD CONSTRAINT user_role_pkey PRIMARY KEY (user_id, role_id);


--
-- Name: usertable usertable_email_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.usertable
    ADD CONSTRAINT usertable_email_key UNIQUE (email);


--
-- Name: usertable usertable_nickname_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.usertable
    ADD CONSTRAINT usertable_nickname_key UNIQUE (nickname);


--
-- Name: usertable usertable_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.usertable
    ADD CONSTRAINT usertable_pkey PRIMARY KEY (id);


--
-- Name: usertable usertable_uuid_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.usertable
    ADD CONSTRAINT usertable_uuid_key UNIQUE (uuid);


--
-- Name: waypoint_symbol waypoint_symbol_pkey; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.waypoint_symbol
    ADD CONSTRAINT waypoint_symbol_pkey PRIMARY KEY (key);


--
-- Name: waypoint_symbol waypoint_symbol_value_key; Type: CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.waypoint_symbol
    ADD CONSTRAINT waypoint_symbol_value_key UNIQUE (value);


--
-- Name: idx_time_inverse; Type: INDEX; Schema: public; Owner: trip
--

CREATE INDEX idx_time_inverse ON public.location USING btree (id, "time" DESC);

ALTER TABLE public.location CLUSTER ON idx_time_inverse;


--
-- Name: itineary_route_point_geog_idx; Type: INDEX; Schema: public; Owner: trip
--

CREATE INDEX itineary_route_point_geog_idx ON public.itinerary_route_point USING gist (geog);


--
-- Name: itineary_track_point_geog_idx; Type: INDEX; Schema: public; Owner: trip
--

CREATE INDEX itineary_track_point_geog_idx ON public.itinerary_track_point USING gist (geog);


--
-- Name: itinerary_waypoint_geog_idx; Type: INDEX; Schema: public; Owner: trip
--

CREATE INDEX itinerary_waypoint_geog_idx ON public.itinerary_waypoint USING gist (geog);


--
-- Name: location_geog_idx; Type: INDEX; Schema: public; Owner: trip
--

CREATE INDEX location_geog_idx ON public.location USING gist (geog);


--
-- Name: itinerary_route itinerary_route_itinerary_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route
    ADD CONSTRAINT itinerary_route_itinerary_id_fkey FOREIGN KEY (itinerary_id) REFERENCES public.itinerary(id) ON DELETE CASCADE;


--
-- Name: itinerary_route_point itinerary_route_point_itinerary_route_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_route_point
    ADD CONSTRAINT itinerary_route_point_itinerary_route_id_fkey FOREIGN KEY (itinerary_route_id) REFERENCES public.itinerary_route(id) ON DELETE CASCADE;


--
-- Name: itinerary_sharing itinerary_sharing_itinerary_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_sharing
    ADD CONSTRAINT itinerary_sharing_itinerary_id_fkey FOREIGN KEY (itinerary_id) REFERENCES public.itinerary(id) ON DELETE CASCADE;


--
-- Name: itinerary_sharing itinerary_sharing_shared_to_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_sharing
    ADD CONSTRAINT itinerary_sharing_shared_to_id_fkey FOREIGN KEY (shared_to_id) REFERENCES public.usertable(id) ON DELETE CASCADE;


--
-- Name: itinerary_track itinerary_track_itinerary_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track
    ADD CONSTRAINT itinerary_track_itinerary_id_fkey FOREIGN KEY (itinerary_id) REFERENCES public.itinerary(id) ON DELETE CASCADE;


--
-- Name: itinerary_track_point itinerary_track_point_itinerary_track_segment_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_point
    ADD CONSTRAINT itinerary_track_point_itinerary_track_segment_id_fkey FOREIGN KEY (itinerary_track_segment_id) REFERENCES public.itinerary_track_segment(id) ON DELETE CASCADE;


--
-- Name: itinerary_track_segment itinerary_track_segment_itinerary_track_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_track_segment
    ADD CONSTRAINT itinerary_track_segment_itinerary_track_id_fkey FOREIGN KEY (itinerary_track_id) REFERENCES public.itinerary_track(id) ON DELETE CASCADE;


--
-- Name: itinerary itinerary_user_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary
    ADD CONSTRAINT itinerary_user_id_fkey FOREIGN KEY (user_id) REFERENCES public.usertable(id);


--
-- Name: itinerary_waypoint itinerary_waypoint_itinerary_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.itinerary_waypoint
    ADD CONSTRAINT itinerary_waypoint_itinerary_id_fkey FOREIGN KEY (itinerary_id) REFERENCES public.itinerary(id) ON DELETE CASCADE;


--
-- Name: location_sharing location_sharing_shared_by_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.location_sharing
    ADD CONSTRAINT location_sharing_shared_by_id_fkey FOREIGN KEY (shared_by_id) REFERENCES public.usertable(id) ON DELETE CASCADE;


--
-- Name: location_sharing location_sharing_shared_to_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.location_sharing
    ADD CONSTRAINT location_sharing_shared_to_id_fkey FOREIGN KEY (shared_to_id) REFERENCES public.usertable(id) ON DELETE CASCADE;


--
-- Name: location location_user_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.location
    ADD CONSTRAINT location_user_id_fkey FOREIGN KEY (user_id) REFERENCES public.usertable(id);


--
-- Name: session_data session_data_session_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.session_data
    ADD CONSTRAINT session_data_session_id_fkey FOREIGN KEY (session_id) REFERENCES public.session(id) ON DELETE CASCADE;


--
-- Name: session session_user_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: trip
--

ALTER TABLE ONLY public.session
    ADD CONSTRAINT session_user_id_fkey FOREIGN KEY (user_id) REFERENCES public.usertable(id) ON DELETE CASCADE;


--
-- Name: SCHEMA public; Type: ACL; Schema: -; Owner: postgres
--

GRANT USAGE ON SCHEMA public TO trip_role;


--
-- Name: TABLE georef_format; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT ON TABLE public.georef_format TO trip_role;


--
-- Name: TABLE itinerary; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary TO trip_role;


--
-- Name: TABLE itinerary_route; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_route TO trip_role;


--
-- Name: TABLE itinerary_route_point; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_route_point TO trip_role;


--
-- Name: SEQUENCE itinerary_route_point_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_route_point_seq TO trip_role;


--
-- Name: SEQUENCE itinerary_route_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_route_seq TO trip_role;


--
-- Name: SEQUENCE itinerary_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_seq TO trip_role;


--
-- Name: TABLE itinerary_sharing; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_sharing TO trip_role;


--
-- Name: TABLE itinerary_track; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_track TO trip_role;


--
-- Name: TABLE itinerary_track_point; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_track_point TO trip_role;


--
-- Name: SEQUENCE itinerary_track_point_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_track_point_seq TO trip_role;


--
-- Name: TABLE itinerary_track_segment; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_track_segment TO trip_role;


--
-- Name: SEQUENCE itinerary_track_segment_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_track_segment_seq TO trip_role;


--
-- Name: SEQUENCE itinerary_track_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_track_seq TO trip_role;


--
-- Name: TABLE itinerary_waypoint; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.itinerary_waypoint TO trip_role;


--
-- Name: SEQUENCE itinerary_waypoint_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.itinerary_waypoint_seq TO trip_role;


--
-- Name: SEQUENCE location_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.location_seq TO trip_role;


--
-- Name: TABLE location; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT ON TABLE public.location TO trip_role;


--
-- Name: TABLE location_sharing; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.location_sharing TO trip_role;


--
-- Name: TABLE path_color; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT ON TABLE public.path_color TO trip_role;


--
-- Name: TABLE role; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT ON TABLE public.role TO trip_role;


--
-- Name: SEQUENCE role_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.role_seq TO trip_role;


--
-- Name: TABLE tile; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.tile TO trip_role;


--
-- Name: SEQUENCE tile_download_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.tile_download_seq TO trip_role;


--
-- Name: TABLE tile_metric; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT ON TABLE public.tile_metric TO trip_role;


--
-- Name: TABLE user_role; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.user_role TO trip_role;


--
-- Name: SEQUENCE usertable_seq; Type: ACL; Schema: public; Owner: trip
--

GRANT USAGE ON SEQUENCE public.usertable_seq TO trip_role;


--
-- Name: TABLE usertable; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT,INSERT,DELETE,UPDATE ON TABLE public.usertable TO trip_role;


--
-- Name: TABLE waypoint_symbol; Type: ACL; Schema: public; Owner: trip
--

GRANT SELECT ON TABLE public.waypoint_symbol TO trip_role;


--
-- PostgreSQL database dump complete
--

