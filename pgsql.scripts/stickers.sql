-- Table: public.stickers

-- DROP TABLE IF EXISTS public.stickers;

CREATE TABLE IF NOT EXISTS public.stickers
(
    id text COLLATE pg_catalog."default" NOT NULL,
    description text COLLATE pg_catalog."default",
    CONSTRAINT stickers_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.stickers
    OWNER to chanserv;