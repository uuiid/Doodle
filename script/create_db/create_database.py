from os import getenv
from sqlalchemy import create_engine
from sqlalchemy.orm import Session
import os
import zou.app
import zou.app.models.project_status
import zou.app.models.organisation
import zou.app.models.studio
import zou.app.models.status_automation
import zou.app.models.entity_type

import doodle.project_status
import doodle.organisation
import doodle.studio
import doodle.status_automation
import doodle.entity_type
import doodle.task_status
PASS = getenv("KITSU_PASS")

def main():
    engine = create_engine("sqlite:///D:\\zou2.db", echo=True)

    with zou.app.app.app_context():
        l_prj_list : list[zou.app.models.project_status.ProjectStatus]= zou.app.models.project_status.ProjectStatus.query.all()
        l_organisation : list[zou.app.models.organisation.Organisation] = zou.app.models.organisation.Organisation.query.all()
        l_studio : list[zou.app.models.studio.Studio] = zou.app.models.studio.Studio.query.all()
        l_status_automation : list[zou.app.models.status_automation.StatusAutomation] = zou.app.models.status_automation.StatusAutomation.query.all()
        l_entity_type : list[zou.app.models.entity_type.EntityType] = zou.app.models.entity_type.EntityType.query.all()
        l_TaskTypeAssetTypeLink : list[zou.app.models.entity_type.TaskTypeAssetTypeLink] = zou.app.models.entity_type.TaskTypeAssetTypeLink.query.all()


        with Session(engine) as session:
            doodle.project_status.ProjectStatus.metadata.create_all(engine)
            doodle.organisation.Organisation.metadata.create_all(engine)
            doodle.studio.Studio.metadata.create_all(engine)
            doodle.status_automation.StatusAutomation.metadata.create_all(engine)
            doodle.entity_type.EntityType.metadata.create_all(engine)
            doodle.task_status.TaskStatus.metadata.create_all(engine)
            doodle.entity_type.TaskTypeAssetTypeLink.metadata.create_all(engine)

            session.add_all([doodle.entity_type.TaskTypeAssetTypeLink().from_zou(i) for i in l_TaskTypeAssetTypeLink])
            session.add_all([doodle.entity_type.EntityType().from_zou(i) for i in l_entity_type])
            session.add_all([doodle.status_automation.StatusAutomation().from_zou(i) for i in l_status_automation])
            session.add_all([doodle.studio.Studio().from_zou(i) for i in l_studio])
            session.add_all([doodle.project_status.ProjectStatus().from_zou(i) for i in l_prj_list])
            session.add_all([doodle.organisation.Organisation().from_zou(i) for i in l_organisation])
            session.commit()



if __name__ == "__main__":
    os.remove("D:\\zou2.db")
    main()
