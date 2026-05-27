-- Table: public.buttons

-- DROP TABLE IF EXISTS public.buttons;

CREATE TABLE IF NOT EXISTS public.buttons
(
    id integer NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 MINVALUE 1 MAXVALUE 2147483647 CACHE 1 ),
    text text COLLATE pg_catalog."default" NOT NULL,
    "callbackData" text COLLATE pg_catalog."default" NOT NULL,
    "row" text COLLATE pg_catalog."default",
    role text COLLATE pg_catalog."default",
    CONSTRAINT buttons_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.buttons
    OWNER to chanserv;