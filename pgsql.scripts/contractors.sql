-- Table: public.contractors

-- DROP TABLE IF EXISTS public.contractors;

CREATE TABLE IF NOT EXISTS public.contractors
(
    id text COLLATE pg_catalog."default" NOT NULL,
    name text COLLATE pg_catalog."default",
    link text COLLATE pg_catalog."default",
    trello_list text COLLATE pg_catalog."default",
    email text COLLATE pg_catalog."default",
    additional text COLLATE pg_catalog."default",
    CONSTRAINT contractors_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.contractors
    OWNER to chanserv;