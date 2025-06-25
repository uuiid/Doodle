from sqlalchemy import create_engine, inspect
from sqlalchemy_utils import database_exists, create_database
from sqlalchemy.engine.url import URL
from sqlalchemy.orm import close_all_sessions


def get_db_uri():
    from zou.app.config import DATABASE

    return URL.create(**DATABASE).render_as_string(hide_password=False)


def reset_all():
    """
    Check that database exist.
    """
    drop_all()
    return create_all()


def create_all():
    """
    Create all database tables.
    """

    return


def drop_all():
    """
    Drop all database tables.
    """

    db.session.flush()
    close_all_sessions()
    return db.drop_all()


def is_init():
    """
    Check if database is initialized.
    """

    from zou.app.models.project_status import ProjectStatus

    return (
            inspect(db.engine).has_table("person")
            and db.session.query(ProjectStatus).count() == 2
    )
